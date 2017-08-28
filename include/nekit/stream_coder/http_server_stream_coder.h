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

#pragma once

#include "../utils/http_header_parser.h"
#include "server_stream_coder_interface.h"

namespace nekit {
namespace stream_coder {
class HttpServerStreamCoder final : public ServerStreamCoderInterface {
 public:
  enum class ErrorCode { NoError = 0, InvalidHeader, InvalidContentLength };

  HttpServerStreamCoder();

  ActionRequest Negotiate() override;

  utils::BufferReserveSize EncodeReserve() const override;
  ActionRequest Encode(utils::Buffer* buffer) override;

  utils::BufferReserveSize DecodeReserve() const override;
  ActionRequest Decode(utils::Buffer* buffer) override;

  std::error_code GetLastError() const override;

  bool forwarding() const override;

  ActionRequest ReportError(std::error_code error) override;

  ActionRequest Continue() override;

  std::shared_ptr<utils::Session> session() const override;

 private:
  bool Process(uint8_t* buffer_head, size_t buffer_len, uint8_t** output_head);
  bool ProcessHeader(uint8_t** output_head, size_t header_size);

  std::shared_ptr<utils::Session> session_;
  std::error_code last_error_;

  std::unique_ptr<uint8_t[]> header_buffer_;
  size_t buffered_header_size_{0};

  std::unique_ptr<uint8_t[]> pending_first_block_;
  size_t pending_first_block_size_{0};

  size_t remaining_length_{0};
  bool first_header_{true};
  bool is_connect_command_{false};
  bool send_response_{false};

  utils::HttpHeaderParser parser_;
};

std::error_code make_error_code(HttpServerStreamCoder::ErrorCode);

class HttpServerStreamCoderFactory : public ServerStreamCoderFactoryInterface {
 public:
  std::unique_ptr<ServerStreamCoderInterface> Build() override;
};
}  // namespace stream_coder
}  // namespace nekit

template <>
struct std::is_error_code_enum<
    nekit::stream_coder::HttpServerStreamCoder::ErrorCode> : std::true_type {};
