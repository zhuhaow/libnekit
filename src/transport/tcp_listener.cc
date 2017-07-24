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

#include "nekit/transport/tcp_listener.h"
#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/boost_error.h"

namespace nekit {
namespace transport {
TcpListener::TcpListener(boost::asio::io_service &io)
    : acceptor_(io), socket_(io) {}

std::error_code TcpListener::Bind(std::string ip, uint16_t port) {
  return Bind(boost::asio::ip::address::from_string(ip), port);
}

std::error_code TcpListener::Bind(boost::asio::ip::address ip, uint16_t port) {
  boost::system::error_code ec;
  boost::asio::ip::tcp::endpoint endpoint(ip, port);

  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    return std::make_error_code(ec);
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    return std::make_error_code(ec);
  }

  return ErrorCode::NoError;
}

void TcpListener::Accept(EventHandler &&handler) {
  acceptor_.async_accept(socket_, [ this, handler{std::move(handler)} ](
                                      const boost::system::error_code &ec) {
    if (ec) {
      handler(nullptr, std::make_error_code(ec));
      return;
    }

    // Can't use `make_unique` since the constructor is a private friend.
    TcpSocket *socket = new TcpSocket(std::move(socket_));
    handler(std::unique_ptr<TcpSocket>(socket),
            TcpListener::ErrorCode::NoError);
  });
}

namespace {
struct TcpListenerErrorCategory : std::error_category {
  const char *name() const noexcept override { return "TCP listener"; }

  std::string message(int ev) const override {
    switch (static_cast<TcpListener::ErrorCode>(ev)) {
      case TcpListener::ErrorCode::NoError:
        return "no error";
    }
  }
};

const TcpListenerErrorCategory tcpListenerErrorCategory{};

}  // namespace

std::error_code make_error_code(TcpListener::ErrorCode ec) {
  return {static_cast<int>(ec), tcpListenerErrorCategory};
}
}  // namespace transport
}  // namespace nekit
