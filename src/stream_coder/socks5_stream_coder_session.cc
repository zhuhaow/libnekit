// MIT License

// Copyright (c) 2017 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>

#include "nekit/stream_coder/socks5_stream_coder_session.h"

namespace nekit {
namespace stream_coder {
const char *SOCKS5StreamCoderSession::ErrorCategory::name() const
    BOOST_NOEXCEPT {
  return "SOCKS5 error.";
}

std::string SOCKS5StreamCoderSession::ErrorCategory::message(
    int error_code) const {
  switch (ErrorCode(error_code)) {
    case kNoError:
      return "No error.";
    case kRequestIncomplete:
      return "Client send incomplete request.";
    case kUnsupportedAuthenticationMethod:
      return "All client requested authentication methods are not "
             "supported.";
    case kUnsupportedCommand:
      return "Unknown command.";
    case kUnsupportedAddressType:
      return "Unknown address type.";
    case kUnsupportedVersion:
      return "SOCKS version is not supported.";
  }
}

const std::error_category &SOCKS5StreamCoderSession::error_category() {
  static ErrorCategory category_;
  return category_;
}

SOCKS5StreamCoderSession::SOCKS5StreamCoderSession(
    std::shared_ptr<utils::Session> session)
    : status_(kReadingVersion), session_(session) {}

ActionRequest SOCKS5StreamCoderSession::Negotiate() { return kWantRead; }

utils::BufferReserveSize SOCKS5StreamCoderSession::InputReserve() const {
  switch (status_) {
    case kForwarding:
    case kReadingVersion:
    case kReadingRequest:
      return {0, 0};
    default:
      assert(false);
  }
}

ActionRequest SOCKS5StreamCoderSession::Input(utils::Buffer *buffer) {
  switch (status_) {
    case kForwarding:
      return kContinue;

    case kReadingVersion: {
      if (buffer->capacity() <= 2) {
        last_error_ = std::make_error_code(kRequestIncomplete);
        return kErrorHappened;
      }

      auto data = static_cast<uint8_t *>(buffer->buffer());
      if (*data++ != 5) {
        last_error_ = std::make_error_code(kUnsupportedVersion);
        return kErrorHappened;
      }

      auto len = *data++;
      if (buffer->capacity() != 2 + len) {
        last_error_ = std::make_error_code(kRequestIncomplete);
        return kErrorHappened;
      }

      bool supported = false;
      while (len--) {
        // Only no auth is supported
        if (*data++ == 0) {
          supported = true;
          break;
        }
      }

      if (!supported) {
        last_error_ = std::make_error_code(kUnsupportedAuthenticationMethod);
        return kErrorHappened;
      }

      return kWantWrite;
    } break;

    case kReadingRequest: {
      if (buffer->capacity() < 10) {
        last_error_ = std::make_error_code(kRequestIncomplete);
        return kErrorHappened;
      }

      auto data = static_cast<uint8_t *>(buffer->buffer());
      if (*data++ != 5) {
        last_error_ = std::make_error_code(kUnsupportedVersion);
        return kErrorHappened;
      }

      if (*data++ != 1) {
        last_error_ = std::make_error_code(kUnsupportedCommand);
        return kErrorHappened;
      }

      data++;

      switch (*data++) {
        case 1: {
          if (buffer->capacity() != 4 + 4 + 2) {
            last_error_ = std::make_error_code(kRequestIncomplete);
            return kErrorHappened;
          }

          auto bytes = boost::asio::ip::address_v4::bytes_type();
          std::memcpy(bytes.data(), data, bytes.size());
          session_->address = boost::asio::ip::address_v4(bytes);
          session_->type = utils::Session::kAddress;
          data += 4;
        } break;
        case 3: {
          auto len = *data++;

          if (buffer->capacity() != 4 + 1 + len + 2) {
            last_error_ = std::make_error_code(kRequestIncomplete);
            return kErrorHappened;
          }

          session_->domain = std::string(reinterpret_cast<char *>(data), len);
          session_->type = utils::Session::kDomain;
          data += len;
        } break;
        case 4: {
          if (buffer->capacity() != 4 + 16 + 2) {
            last_error_ = std::make_error_code(kRequestIncomplete);
            return kErrorHappened;
          }

          auto bytes = boost::asio::ip::address_v6::bytes_type();
          std::memcpy(bytes.data(), data, bytes.size());
          session_->address = boost::asio::ip::address_v6(bytes);
          session_->type = utils::Session::kAddress;
          data += 16;
        } break;
        default: {
          last_error_ = std::make_error_code(kUnsupportedAddressType);
          return kErrorHappened;
        }
      }

      session_->port = ntohs(*reinterpret_cast<uint16_t *>(data));
      return kEvent;
    }
  }
}

utils::BufferReserveSize SOCKS5StreamCoderSession::OutputReserve() const {
  switch (status_) {
    case kForwarding:
      return {0, 0};
    case kReadingVersion:
      return {0, 2};
    case kReadingRequest:
      switch (session_->type) {
        case utils::Session::kDomain:
          return {0, 10};
        case utils::Session::kAddress:
          if (session_->address.is_v4()) {
            return {0, 10};
          } else {
            return {0, 22};
          }
      }
  }
}

ActionRequest SOCKS5StreamCoderSession::Output(utils::Buffer *buffer) {
  switch (status_) {
    case kForwarding:
      return kContinue;
    case kReadingVersion: {
      buffer->ReleaseBack(2);
      assert(buffer->capacity() == 2);
      auto data = static_cast<uint8_t *>(buffer->buffer());
      *data = 5;
      *(data + 1) = 0;
      status_ = kReadingRequest;
      return kWantRead;
    }
    case kReadingRequest: {
      std::size_t len;
      uint8_t type;
      switch (session_->type) {
        case utils::Session::kDomain:
          len = 10;
          type = 1;
          break;
        case utils::Session::kAddress:
          if (session_->address.is_v4()) {
            len = 10;
            type = 1;
          } else {
            len = 22;
            type = 4;
          }
          break;
      }
      buffer->ReleaseBack(len);

      auto data = static_cast<uint8_t *>(buffer->buffer());
      *data = 5;
      *(data + 1) = 0;
      *(data + 2) = 0;
      *(data + 3) = type;
      std::memset(data + 4, 0, len - 4);

      return kReady;
    }
  }
}

utils::Error SOCKS5StreamCoderSession::GetLatestError() const {
  return last_error_;
}

bool SOCKS5StreamCoderSession::forwarding() const {
  return status_ == kForwarding;
}

ActionRequest SOCKS5StreamCoderSession::Continue(utils::Error error) {
  if (error) {
    last_error_ = error;
    return kErrorHappened;
  }
  return kWantWrite;
}

}  // namespace stream_coder
}  // namespace nekit

namespace std {

error_code make_error_code(
    nekit::stream_coder::SOCKS5StreamCoderSession::ErrorCode errc) {
  return error_code(
      static_cast<int>(errc),
      nekit::stream_coder::SOCKS5StreamCoderSession::error_category());
}
}  // namespace std
