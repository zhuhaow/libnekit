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

#include "nekit/transport/tcp_connector.h"
#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/boost_error.h"
#include "nekit/utils/no_error.h"

namespace nekit {
namespace transport {
TcpConnector::TcpConnector(const boost::asio::ip::address& address,
                           uint16_t port, boost::asio::io_service& io)
    : socket_{io}, address_{address}, port_{port} {}

TcpConnector::TcpConnector(
    std::shared_ptr<std::vector<boost::asio::ip::address>> addresses,
    uint16_t port, boost::asio::io_service& io)
    : socket_{io}, addresses_{addresses}, port_{port} {}

void TcpConnector::Connect(EventHandler&& handler) {
  assert(!connecting_);

  handler_ = std::move(handler);
  current_ind_ = 0;

  DoConnect();
}

void TcpConnector::DoConnect() {
  connecting_ = true;

  const boost::asio::ip::address* address;
  if (current_ind_ == 0 && addresses_->size() == 0) {
    address = &address_;
  } else {
    if (current_ind_ >= addresses_->size()) {
      handler_(nullptr, last_error_);
      return;
    }
    address = &addresses_->at(current_ind_);
  }

  socket_.async_connect(boost::asio::ip::tcp::endpoint(*address, port_),
                        [this](const boost::system::error_code& ec) {
                          if (ec) {
                            last_error_ = std::make_error_code(ec);
                            current_ind_++;
                            DoConnect();
                            return;
                          }

                          connecting_ = false;
                          handler_(
                              std::make_unique<TcpSocket>(std::move(socket_)),
                              utils::NEKitErrorCode::NoError);
                          return;
                        });
}
}  // namespace transport
}  // namespace nekit
