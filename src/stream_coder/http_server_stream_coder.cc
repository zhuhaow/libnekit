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

#include "nekit/stream_coder/http_server_stream_coder.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/assert.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/lexical_cast.hpp>

namespace nekit {
namespace stream_coder {

const std::vector<std::string> RemoveHeaders = {"Proxy-Authenticate",
                                                "Proxy-Authorization"};

const std::string ConnectResponse =
    "HTTP/1.1 200 Connection Established\r\n\r\n";

HttpServerStreamCoder::HttpServerStreamCoder()
    : header_buffer_{
          std::make_unique<uint8_t[]>(NEKIT_HTTP_HEADER_MAX_LENGTH)} {}

ActionRequest HttpServerStreamCoder::Negotiate() {
  return ActionRequest::WantRead;
}

utils::BufferReserveSize HttpServerStreamCoder::EncodeReserve() const {
  if (is_connect_command_ && !send_response_) {
    return {ConnectResponse.size(), 0};
  }
  return {0, 0};
}

ActionRequest HttpServerStreamCoder::Encode(utils::Buffer* buffer) {
  if (is_connect_command_ && !send_response_) {
    send_response_ = true;
    buffer->ReleaseFront(ConnectResponse.size());
    memcpy(buffer->buffer(), ConnectResponse.c_str(), ConnectResponse.size());
    return ActionRequest::Ready;
  }

  return ActionRequest::Continue;
}

utils::BufferReserveSize HttpServerStreamCoder::DecodeReserve() const {
  return {buffered_header_size_ + pending_first_block_size_, 0};
}

ActionRequest HttpServerStreamCoder::Decode(utils::Buffer* buffer) {
  if (is_connect_command_) {
    return ActionRequest::Continue;
  }

  uint8_t* buffer_head = static_cast<uint8_t*>(buffer->buffer());
  size_t buffer_len = buffer->size();

  buffer->ReleaseFront(buffered_header_size_ + pending_first_block_size_);
  uint8_t* output_head = static_cast<uint8_t*>(buffer->buffer());
  if (pending_first_block_size_) {
    memcpy(buffer->buffer(), pending_first_block_.get(),
           pending_first_block_size_);

    output_head += pending_first_block_size_;
    pending_first_block_size_ = 0;
    pending_first_block_ = nullptr;
  }

  if (!Process(buffer_head, buffer_len, &output_head)) {
    return ActionRequest::ErrorHappened;
  }

  ptrdiff_t distance =
      static_cast<uint8_t*>(buffer->buffer()) + buffer->size() - output_head;
  BOOST_ASSERT(distance >= 0);
  buffer->ReserveBack(distance);

  if (first_header_) {
    if (session_) {
      if (!is_connect_command_) {
        pending_first_block_ = std::make_unique<uint8_t[]>(buffer->size());
        pending_first_block_size_ = buffer->size();
        memcpy(pending_first_block_.get(), buffer->buffer(), buffer->size());
      }
      return ActionRequest::Event;
    } else {
      return ActionRequest::WantRead;
    }
  }

  return ActionRequest::Continue;
}

bool HttpServerStreamCoder::Process(uint8_t* buffer_head, size_t buffer_len,
                                    uint8_t** output_head) {
  while (buffer_len) {
    if (remaining_length_) {
      size_t content_length = std::min(buffer_len, remaining_length_);
      if (buffer_head != *output_head) {
        memmove(*output_head, buffer_head, content_length);
      }
      (*output_head) += content_length;
      buffer_head += content_length;
      buffer_len -= content_length;
      remaining_length_ -= content_length;
    } else {
      size_t block_length = std::min(
          NEKIT_HTTP_HEADER_MAX_LENGTH - buffered_header_size_, buffer_len);
      memcpy(header_buffer_.get() + buffered_header_size_, buffer_head,
             block_length);
      buffered_header_size_ += block_length;
      buffer_head += block_length;
      buffer_len -= block_length;

      int result = parser_.Parse(header_buffer_.get(), buffered_header_size_);
      switch (result) {
        case utils::HttpHeaderParser::ParseError:
          last_error_ = ErrorCode::InvalidHeader;
          return false;
        case utils::HttpHeaderParser::HeaderIncomplete:
          return true;
        default:
          BOOST_ASSERT(result > 0);
          if (!ProcessHeader(output_head, result)) {
            return false;
          }

          block_length = buffered_header_size_ - result;
          memcpy(*output_head, header_buffer_.get() + result, block_length);
          (*output_head) += block_length;
          if (first_header_) {
            session_ = std::make_shared<utils::Session>(parser_.host(),
                                                        parser_.port());
          }
          buffered_header_size_ = 0;
          return true;
      }
    }
  }

  return true;
}

bool HttpServerStreamCoder::ProcessHeader(uint8_t** output_head,
                                          size_t header_size) {
  if (first_header_) {
    if (boost::iequals(parser_.method(), "CONNECT")) {
      is_connect_command_ = true;
      return true;
    }
  }

  boost::iostreams::stream_buffer<boost::iostreams::array_sink> buf(
      reinterpret_cast<char*>(*output_head), header_size);
  std::ostream os(&buf);

  auto init_pos = os.tellp();

  os << parser_.method() << " " << parser_.relative_path() << " "
     << "HTTP/1." << parser_.minor_version() << "\r\n";

  struct phr_header* headers = parser_.header_fields();

  bool found_connection = false, found_length = false;

  for (size_t i = 0; i < parser_.header_field_size(); i++) {
    std::string name(headers[i].name, headers[i].name_len);

    bool skip = false;
    for (const auto& n : RemoveHeaders) {
      if (boost::iequals(name, n)) {
        skip = true;
        break;
      }
    }

    if (skip) {
      continue;
    }

    if (!found_connection && boost::iequals(name, "Proxy-Connection")) {
      found_connection = true;
      name = "Connection";
    }

    if (!found_length && boost::iequals(name, "Content-Length")) {
      found_length = true;
      try {
        remaining_length_ = boost::lexical_cast<size_t>(
            std::string(headers[i].value, headers[i].value_len));
      } catch (...) {
        last_error_ = ErrorCode::InvalidContentLength;
        return false;
      }
    }
    os << name << ": " << std::string(headers[i].value, headers[i].value_len)
       << "\r\n";
  }
  os << "\r\n";

  (*output_head) += (os.tellp() - init_pos);

  return true;
}

std::error_code HttpServerStreamCoder::GetLastError() const {
  return last_error_;
}

bool HttpServerStreamCoder::forwarding() const { return true; }

ActionRequest HttpServerStreamCoder::ReportError(std::error_code error) {
  last_error_ = error;
  return ActionRequest::ErrorHappened;
}

ActionRequest HttpServerStreamCoder::Continue() {
  BOOST_ASSERT(first_header_);
  first_header_ = false;

  if (is_connect_command_) {
    return ActionRequest::WantWrite;
  }
  return ActionRequest::ReadyAfterRead;
}

std::shared_ptr<utils::Session> HttpServerStreamCoder::session() const {
  return session_;
}

namespace {
struct HttpServerStreamCoderErrorCategory : std::error_category {
  const char* name() const noexcept override {
    return "Http server stream coder";
  }

  std::string message(int error_code) const override {
    switch (static_cast<HttpServerStreamCoder::ErrorCode>(error_code)) {
      case HttpServerStreamCoder::ErrorCode::NoError:
        return "no error";
      case HttpServerStreamCoder::ErrorCode::InvalidHeader:
        return "invalid http request header";
      case HttpServerStreamCoder::ErrorCode::InvalidContentLength:
        return "content-length is not valid";
    }
  }
};

const HttpServerStreamCoderErrorCategory httpServerStreamCoderErrorCategory{};
}  // namespace

std::error_code make_error_code(HttpServerStreamCoder::ErrorCode ec) {
  return {static_cast<int>(ec), httpServerStreamCoderErrorCategory};
}

}  // namespace stream_coder
}  // namespace nekit
