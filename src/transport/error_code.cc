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

#include "nekit/transport/error_code.h"

namespace nekit {
namespace transport {
namespace {
struct TransportErrorCategory : std::error_category {
  const char *name() const noexcept override;
  std::string message(int) const override;
};

const char *TransportErrorCategory::name() const noexcept {
  return "Transport";
}

std::string TransportErrorCategory::message(int ev) const {
  switch (static_cast<ErrorCode>(ev)) {
    case ErrorCode::NoError:
      return "no error";
    case ErrorCode::Closed:
      return "closed";
    case ErrorCode::ConnectionAborted:
      return "connection aborted";
    case ErrorCode::ConnectionReset:
      return "connection reset";
    case ErrorCode::HostUnreachable:
      return "host unreachable";
    case ErrorCode::NetworkDown:
      return "network down";
    case ErrorCode::NetworkReset:
      return "network reset";
    case ErrorCode::NetworkUnreachable:
      return "network unreachable";
    case ErrorCode::TimedOut:
      return "timeout";
    case ErrorCode::EndOfFile:
      return "end of file";
    case ErrorCode::UnknownError:
      return "unknown error";
  }
}

const TransportErrorCategory transportErrorCategory{};

}  // namespace

std::error_code make_error_code(ErrorCode e) {
  return {static_cast<int>(e), transportErrorCategory};
}
}  // namespace transport
}  // namespace nekit
