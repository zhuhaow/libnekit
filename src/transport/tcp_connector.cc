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
#include "nekit/utils/runtime.h"

namespace nekit {
namespace transport {
TcpConnector::TcpConnector(const boost::asio::ip::address& address,
                           uint16_t port, boost::asio::io_service& io)
    : socket_{io},
      addresses_{std::make_shared<std::vector<boost::asio::ip::address>>(
          std::vector<boost::asio::ip::address>{address})},
      port_{port} {}

TcpConnector::TcpConnector(
    std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
    uint16_t port, boost::asio::io_service& io)
    : socket_{io}, addresses_{addresses}, port_{port} {
  assert(!addresses_->empty());
}

TcpConnector::TcpConnector(std::shared_ptr<utils::Domain> domain, uint16_t port,
                           boost::asio::io_service& io)
    : socket_{io}, domain_{domain}, port_{port} {}

TcpConnector::TcpConnector(std::string domain, uint16_t port,
                           boost::asio::io_service& io)
    : TcpConnector(std::make_shared<utils::Domain>(domain), port, io) {}

void TcpConnector::Connect(EventHandler&& handler) {
  assert(!connecting_);

  handler_ = std::move(handler);

  if (!addresses_) {
    if (domain_->isAddressAvailable()) {
      addresses_ = domain_->addresses();
    } else {
      if (domain_->isResolved()) {
        handler_(nullptr, domain_->error());
        return;
      } else {
        domain_->Resolve([this](std::error_code ec) {
          if (ec) {
            handler_(nullptr, ec);
            return;
          }

          addresses_ = domain_->addresses();
          DoConnect();
        });
          return;
      }
    }
  }

  DoConnect();
}

void TcpConnector::Bind(std::shared_ptr<utils::DeviceInterface> device) {
  device_ = device;
}

void TcpConnector::DoConnect() {
  connecting_ = true;

  if (current_ind_ >= addresses_->size()) {
    handler_(nullptr, last_error_);
    return;
  }

  const auto& address = addresses_->at(current_ind_);
  socket_.async_connect(
      boost::asio::ip::tcp::endpoint(address, port_),
      [this](const boost::system::error_code& ec) {
        if (ec) {
          last_error_ = std::make_error_code(ec);
          current_ind_++;
          DoConnect();
          return;
        }

        connecting_ = false;
        handler_(std::unique_ptr<TcpSocket>(new TcpSocket(std::move(socket_))),
                 utils::NEKitErrorCode::NoError);
        return;
      });
}

TcpConnectorFactory::TcpConnectorFactory(boost::asio::io_service& io)
    : io_{&io} {}

std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
    const boost::asio::ip::address& address, uint16_t port) {
  return std::make_unique<TcpConnector>(address, port, *io_);
}

std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
    std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
    uint16_t port) {
  return std::make_unique<TcpConnector>(addresses, port, *io_);
}

std::unique_ptr<ConnectorInterface> TcpConnectorFactory::Build(
    std::shared_ptr<utils::Domain> domain, uint16_t port) {
  return std::make_unique<TcpConnector>(domain, port, *io_);
}
}  // namespace transport
}  // namespace nekit
