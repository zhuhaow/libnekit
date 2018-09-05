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
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "TCP Listener"

namespace nekit {
namespace transport {
TcpListener::TcpListener(utils::Runloop *runloop, DataFlowHandler handler)
    : acceptor_(*runloop->BoostIoContext()),
      socket_(*runloop->BoostIoContext()),
      runloop_{runloop},
      handler_{handler} {}

utils::Result<void> TcpListener::Bind(std::string ip, uint16_t port) {
  return Bind(boost::asio::ip::address::from_string(ip), port);
}

utils::Result<void> TcpListener::Bind(boost::asio::ip::address ip,
                                      uint16_t port) {
  boost::system::error_code ec;
  utils::Error error;
  boost::asio::ip::tcp::endpoint endpoint(ip, port);

  NEDEBUG << "Trying to bind listener to " << endpoint << ".";

  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    error = utils::BoostErrorCategory::FromBoostError(ec);
    NEERROR << "Failed to open listener due to " << ec << ".";
    return utils::MakeErrorResult(std::move(error));
  }

  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec) {
    error = utils::BoostErrorCategory::FromBoostError(ec);
    NEERROR << "Failed to set listener to reuse address due to " << error
            << ".";
    return utils::MakeErrorResult(std::move(error));
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    utils::Error error;
    if (ec.value() == boost::system::errc::address_in_use) {
      error = utils::Error(ListenerErrorCategory::GlobalListenerErrorCategory(),
                           (int)ListenerErrorCode::AddressInUse);
    } else {
      error = utils::BoostErrorCategory::FromBoostError(ec);
    }
    NEERROR << "Failed to bind listener to " << endpoint << " due to " << error
            << ".";
    return utils::MakeErrorResult(std::move(error));
  }

  acceptor_.listen(8, ec);
  if (ec) {
    error = utils::BoostErrorCategory::FromBoostError(ec);
    NEERROR << "Failed to set listener to listen state due to " << error << ".";
    return utils::MakeErrorResult(std::move(error));
  }

  NEINFO << "Successfully bind TCP listener to " << endpoint << ".";

  return {};
}

void TcpListener::Accept(EventHandler handler) {
  NEDEBUG << "Start accepting new socket.";

  acceptor_.async_accept(
      socket_, [this, handler](const boost::system::error_code &ec) {
        if (ec) {
          if (ec.value() == boost::asio::error::operation_aborted) {
            return;
          }

          utils::Error error = utils::BoostErrorCategory::FromBoostError(ec);
          NEERROR << "Failed to accept new socket due to " << error << ".";

          handler(utils::MakeErrorResult(std::move(error)));
          return;
        }

        NEINFO << "Accepted new TCP socket.";

        // Can't use `make_unique` since the constructor is a private friend.
        TcpSocket *socket = new TcpSocket(
            std::move(socket_), std::make_shared<utils::Session>(runloop_));

        handler(handler_(std::unique_ptr<TcpSocket>(socket)));

        Accept(handler);
      });
}

void TcpListener::Close() { acceptor_.close(); }

utils::Runloop *TcpListener::GetRunloop() { return runloop_; }

}  // namespace transport
}  // namespace nekit
