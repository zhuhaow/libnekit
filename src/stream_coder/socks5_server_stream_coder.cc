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

#include "nekit/stream_coder/socks5_server_stream_coder.h"

namespace nekit {
namespace stream_coder {
Socks5ServerStreamCoder::Socks5ServerStreamCoder()
    : status_(Status::ReadingVersion) {}

ActionRequest Socks5ServerStreamCoder::Negotiate() {
  return ActionRequest::WantRead;
}

utils::BufferReserveSize Socks5ServerStreamCoder::DecodeReserve() const {
  switch (status_) {
    case Status::Forwarding:
    case Status::ReadingVersion:
    case Status::ReadingRequest:
      return {0, 0};
    default:
      assert(false);
  }
}

ActionRequest Socks5ServerStreamCoder::Decode(utils::Buffer *buffer) {
  switch (status_) {
    case Status::Forwarding:
      return ActionRequest::Continue;

    case Status::ReadingVersion: {
      if (buffer->size() <= 2) {
        last_error_ = ErrorCode::RequestIncomplete;
        return ActionRequest::ErrorHappened;
      }

      auto data = static_cast<uint8_t *>(buffer->buffer());
      if (*data++ != 5) {
        last_error_ = ErrorCode::UnsupportedVersion;
        return ActionRequest::ErrorHappened;
      }

      auto len = *data++;
      if (buffer->size() != 2 + len) {
        last_error_ = ErrorCode::RequestIncomplete;
        return ActionRequest::ErrorHappened;
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
        last_error_ = ErrorCode::UnsupportedAuthenticationMethod;
        return ActionRequest::ErrorHappened;
      }

      return ActionRequest::WantWrite;
    } break;

    case Status::ReadingRequest: {
      if (buffer->size() < 10) {
        last_error_ = ErrorCode::RequestIncomplete;
        return ActionRequest::ErrorHappened;
      }

      auto data = static_cast<uint8_t *>(buffer->buffer());
      if (*data++ != 5) {
        last_error_ = ErrorCode::UnsupportedVersion;
        return ActionRequest::ErrorHappened;
      }

      if (*data++ != 1) {
        last_error_ = ErrorCode::UnsupportedCommand;
        return ActionRequest::ErrorHappened;
      }

      data++;

      switch (*data++) {
        case 1: {
          if (buffer->size() != 4 + 4 + 2) {
            last_error_ = ErrorCode::RequestIncomplete;
            return ActionRequest::ErrorHappened;
          }

          auto bytes = boost::asio::ip::address_v4::bytes_type();
          std::memcpy(bytes.data(), data, bytes.size());
          session_.reset(
              new utils::Session(boost::asio::ip::address_v4(bytes)));
          data += 4;
        } break;
        case 3: {
          auto len = *data++;

          if (buffer->size() != 4 + 1 + len + 2) {
            last_error_ = ErrorCode::RequestIncomplete;
            return ActionRequest::ErrorHappened;
          }

          session_.reset(new utils::Session(
              std::string(reinterpret_cast<char *>(data), len)));
          data += len;
        } break;
        case 4: {
          if (buffer->size() != 4 + 16 + 2) {
            last_error_ = ErrorCode::RequestIncomplete;
            return ActionRequest::ErrorHappened;
          }

          auto bytes = boost::asio::ip::address_v6::bytes_type();
          std::memcpy(bytes.data(), data, bytes.size());
          session_.reset(
              new utils::Session(boost::asio::ip::address_v6(bytes)));
          data += 16;
        } break;
        default: {
          last_error_ = ErrorCode::UnsupportedAddressType;
          return ActionRequest::ErrorHappened;
        }
      }

      session_->endpoint()->set_port(ntohs(*reinterpret_cast<uint16_t *>(data)));
      return ActionRequest::Event;
    }
  }
}

utils::BufferReserveSize Socks5ServerStreamCoder::EncodeReserve() const {
  switch (status_) {
    case Status::Forwarding:
      return {0, 0};
    case Status::ReadingVersion:
      return {0, 2};
    case Status::ReadingRequest:
      switch (session_->endpoint()->type()) {
        case utils::Endpoint::Type::Domain:
          return {0, 10};
        case utils::Endpoint::Type::Address:
          if (session_->endpoint()->address().is_v4()) {
            return {0, 10};
          } else {
            return {0, 22};
          }
      }
  }
}

ActionRequest Socks5ServerStreamCoder::Encode(utils::Buffer *buffer) {
  switch (status_) {
    case Status::Forwarding:
      return ActionRequest::Continue;
    case Status::ReadingVersion: {
      buffer->ReleaseBack(2);
      assert(buffer->size() >= 2);
      auto data = static_cast<uint8_t *>(buffer->buffer());
      *data = 5;
      *(data + 1) = 0;
      status_ = Status::ReadingRequest;
      return ActionRequest::WantRead;
    }
    case Status::ReadingRequest: {
      std::size_t len;
      uint8_t type;
      switch (session_->endpoint()->type()) {
        case utils::Endpoint::Type::Domain:
          len = 10;
          type = 1;
          break;
        case utils::Endpoint::Type::Address:
          if (session_->endpoint()->address().is_v4()) {
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

      status_ = Status::Forwarding;

      return ActionRequest::Ready;
    }
  }
}

std::error_code Socks5ServerStreamCoder::GetLastError() const {
  return last_error_;
}

bool Socks5ServerStreamCoder::forwarding() const {
  return status_ == Status::Forwarding;
}

ActionRequest Socks5ServerStreamCoder::ReportError(std::error_code error) {
  // It is possible to report it elegantly.
  last_error_ = error;
  return ActionRequest::ErrorHappened;
}

ActionRequest Socks5ServerStreamCoder::Continue() {
  return ActionRequest::WantWrite;
}

std::shared_ptr<utils::Session> Socks5ServerStreamCoder::session() const {
  return session_;
}

namespace {
struct Socks5ServerStreamCoderErrorCategory : std::error_category {
  const char *name() const noexcept override;
  std::string message(int) const override;
};

const char *Socks5ServerStreamCoderErrorCategory::name() const BOOST_NOEXCEPT {
  return "SOCKS5 server stream coder";
}

std::string Socks5ServerStreamCoderErrorCategory::message(
    int error_code) const {
  switch (static_cast<Socks5ServerStreamCoder::ErrorCode>(error_code)) {
    case Socks5ServerStreamCoder::ErrorCode::NoError:
      return "no error";
    case Socks5ServerStreamCoder::ErrorCode::RequestIncomplete:
      return "client send incomplete request";
    case Socks5ServerStreamCoder::ErrorCode::UnsupportedAuthenticationMethod:
      return "all client requested authentication methods are not "
             "supported";
    case Socks5ServerStreamCoder::ErrorCode::UnsupportedCommand:
      return "unknown command";
    case Socks5ServerStreamCoder::ErrorCode::UnsupportedAddressType:
      return "unknown address type";
    case Socks5ServerStreamCoder::ErrorCode::UnsupportedVersion:
      return "SOCKS version is not supported";
  }
}

const Socks5ServerStreamCoderErrorCategory
    socks5ServerStreamCoderErrorCategory{};

}  // namespace

std::error_code make_error_code(Socks5ServerStreamCoder::ErrorCode ec) {
  return {static_cast<int>(ec), socks5ServerStreamCoderErrorCategory};
}

}  // namespace stream_coder
}  // namespace nekit
