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

#include "nekit/transport/connection_interface.h"

namespace nekit {
namespace transport {
namespace {
struct ConnectionInterfaceErrorCategory : std::error_category {
  const char *name() const noexcept override;
  std::string message(int) const override;
};

const char *ConnectionInterfaceErrorCategory::name() const noexcept {
  return "Connection";
}

std::string ConnectionInterfaceErrorCategory::message(int ev) const {
  switch (static_cast<ConnectionInterface::ErrorCode>(ev)) {
    case ConnectionInterface::ErrorCode::NoError:
      return "no error";
    case ConnectionInterface::ErrorCode::Closed:
      return "closed";
    case ConnectionInterface::ErrorCode::ConnectionAborted:
      return "connection aborted";
    case ConnectionInterface::ErrorCode::ConnectionReset:
      return "connection reset";
    case ConnectionInterface::ErrorCode::HostUnreachable:
      return "host unreachable";
    case ConnectionInterface::ErrorCode::NetworkDown:
      return "network down";
    case ConnectionInterface::ErrorCode::NetworkReset:
      return "network reset";
    case ConnectionInterface::ErrorCode::NetworkUnreachable:
      return "network unreachable";
    case ConnectionInterface::ErrorCode::TimedOut:
      return "timeout";
    case ConnectionInterface::ErrorCode::EndOfFile:
      return "end of file";
    case ConnectionInterface::ErrorCode::UnknownError:
      return "unknown error";
  }
}

const ConnectionInterfaceErrorCategory connectionInterfaceErrorCategory{};

}  // namespace

std::error_code make_error_code(ConnectionInterface::ErrorCode e) {
  return {static_cast<int>(e), connectionInterfaceErrorCategory};
}
}  // namespace transport
}  // namespace nekit
