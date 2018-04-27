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
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "TCP Connector"

namespace nekit {
namespace transport {
TcpConnector::TcpConnector(
    std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
    uint16_t port, boost::asio::io_context* io)
    : socket_{*io}, addresses_{addresses}, port_{port} {
  assert(!addresses_->empty());
}

TcpConnector::TcpConnector(const boost::asio::ip::address& address,
                           uint16_t port, boost::asio::io_context* io)
    : socket_{*io}, address_{address}, port_{port} {}

TcpConnector::TcpConnector(std::shared_ptr<utils::Endpoint> endpoint,
                           boost::asio::io_context* io)
    : socket_{*io}, endpoint_{endpoint}, port_{endpoint->port()} {}

utils::Cancelable TcpConnector::Connect(EventHandler handler) {
  assert(!connecting_);

  NEDEBUG << "Begin connecting to remote.";

  if (endpoint_) {
    if (endpoint_->IsAddressAvailable()) {
      switch (endpoint_->type()) {
        case utils::Endpoint::Type::Address:
          address_ = endpoint_->address();
          break;
        case utils::Endpoint::Type::Domain:
          addresses_ = endpoint_->resolved_addresses();
          break;
      }

      NEDEBUG << "Addresses are available, connect directly.";
      DoConnect(handler);
      // Note connector is disposable.
      return life_time_cancelable();
    } else {
      if (!endpoint_->IsResolvable()) {
        NEERROR << "Can not connect since resolve is failed due to "
                << endpoint_->resolve_error() << ".";

        boost::asio::post(
            socket_.get_executor(),
            [this, handler, cancelable{life_time_cancelable()}]() {
              if (cancelable.canceled()) {
                return;
              }

              handler(std::move(socket_), endpoint_->resolve_error());
            });
        return life_time_cancelable();
      } else {
        (void)endpoint_->Resolve(
            [this, handler,
             cancelable{life_time_cancelable()}](std::error_code ec) mutable {
              if (cancelable.canceled()) {
                return;
              }

              if (ec) {
                NEERROR << "Can not connect since resolve is failed due to "
                        << endpoint_->resolve_error() << ".";
                handler(std::move(socket_), ec);
                return;
              }

              NEDEBUG << "Domain resolved, connect now.";
              addresses_ = endpoint_->resolved_addresses();
              DoConnect(handler);
            });
        return life_time_cancelable();
      }
    }
  }

  NEDEBUG << "Connect request made by addresses, connect directly.";

  DoConnect(handler);

  return life_time_cancelable();
}

void TcpConnector::Bind(std::shared_ptr<utils::DeviceInterface> device) {
  device_ = device;
}

void TcpConnector::DoConnect(EventHandler handler) {
  connecting_ = true;

  if (life_time_cancelable().canceled()) {
    connecting_ = false;
    return;
  }

  if ((!addresses_ && current_ind_) ||
      (addresses_ && current_ind_ >= addresses_->size())) {
    NEERROR << "Fail to connect to all addresses, the last known error is "
            << last_error_ << ".";
    handler(std::move(socket_), last_error_);
    return;
  }

  boost::system::error_code ec;
  socket_.close(ec);
  assert(!ec);

  const boost::asio::ip::address* address;
  if (addresses_) {
    address = &addresses_->at(current_ind_);
  } else {
    address = &address_;
  }

  socket_.async_connect(
      boost::asio::ip::tcp::endpoint(*address, port_),
      [this, handler, cancelable{life_time_cancelable()}](
          const boost::system::error_code& ec) mutable {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          NEDEBUG << "Connect failed due to " << ec << ", trying next address.";

          if (ec.value() == boost::asio::error::operation_aborted) {
            return;
          }

          last_error_ = std::make_error_code(ec);
          current_ind_++;
          DoConnect(handler);
          return;
        }

        NEINFO << "Successfully connected to remote.";
        connecting_ = false;
        handler(std::move(socket_), utils::NEKitErrorCode::NoError);
        return;
      });
}

boost::asio::io_context* TcpConnector::io() {
  return &socket_.get_io_context();
}

// TcpConnectorFactory::TcpConnectorFactory(boost::asio::io_context* io)
//     : ConnectorFactoryInterface{io} {}

// std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
//     const boost::asio::ip::address& address, uint16_t port) {
//   return std::make_unique<TcpConnector>(address, port, io());
// }

// std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
//     std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
//     uint16_t port) {
//   return std::make_unique<TcpConnector>(addresses, port, io());
// }

// std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
//     std::shared_ptr<utils::Endpoint> endpoint) {
//   return std::make_unique<TcpConnector>(endpoint, io());
// }
}  // namespace transport
}  // namespace nekit
