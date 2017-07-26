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

#include <array>
#include <system_error>

#include "server_stream_coder_interface.h"

namespace nekit {
namespace stream_coder {
class Socks5ServerStreamCoder final : public ServerStreamCoderInterface {
 public:
  enum class ErrorCode {
    NoError = 0,
    RequestIncomplete,
    UnsupportedVersion,
    UnsupportedAuthenticationMethod,
    UnsupportedCommand,
    UnsupportedAddressType
  };

  Socks5ServerStreamCoder();

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
  enum class Status { ReadingVersion, ReadingRequest, Forwarding };

  Status status_;
  std::shared_ptr<utils::Session> session_;
  std::error_code last_error_;
};

std::error_code make_error_code(Socks5ServerStreamCoder::ErrorCode ec);

class Socks5ServerStreamCoderFactory : public ServerStreamCoderFactory {
 public:
  std::unique_ptr<ServerStreamCoderInterface> Build() override;
};
}  // namespace stream_coder
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<
    nekit::stream_coder::Socks5ServerStreamCoder::ErrorCode>
    : public std::true_type {};
}  // namespace std
