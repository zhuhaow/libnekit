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

#include "../utils/error.h"
#include "stream_coder_session_interface.h"

namespace nekit {
namespace stream_coder {
class SOCKS5StreamCoderSession final : public StreamCoderSessionInterface {
 public:
  enum ErrorCode {
    kNoError = 0,
    kRequestIncomplete,
    kUnsupportedVersion,
    kUnsupportedAuthenticationMethod,
    kUnsupportedCommand,
    kUnsupportedAddressType
  };

  class ErrorCategory final : public std::error_category {
    const char* name() const BOOST_NOEXCEPT override;
    std::string message(int error_code) const override;
  };

  const static std::error_category& error_category();

  SOCKS5StreamCoderSession(std::shared_ptr<utils::Session> session);
  ~SOCKS5StreamCoderSession() {}

  ActionRequest Negotiate();

  utils::BufferReserveSize InputReserve() const;
  ActionRequest Input(utils::Buffer* buffer);

  utils::BufferReserveSize OutputReserve() const;
  ActionRequest Output(utils::Buffer* buffer);

  utils::Error GetLatestError() const;

  bool forwarding() const;

  ActionRequest Continue(utils::Error error);

 private:
  enum Status { kReadingVersion, kReadingRequest, kForwarding };

  Status status_;
  std::shared_ptr<utils::Session> session_;
  utils::Error last_error_;
};
}  // namespace stream_coder
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<
    nekit::stream_coder::SOCKS5StreamCoderSession::ErrorCode>
    : public std::true_type {};

error_code make_error_code(
    nekit::stream_coder::SOCKS5StreamCoderSession::ErrorCode errc);
}  // namespace std
