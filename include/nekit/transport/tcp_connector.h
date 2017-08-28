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

#include <vector>

#include <boost/asio.hpp>

#include "connector_interface.h"

namespace nekit {
namespace transport {

class TcpConnector;

class TcpConnectorFactory : public ConnectorFactoryInterface {
 public:
  explicit TcpConnectorFactory(boost::asio::io_service& io);

  std::unique_ptr<ConnectorInterface> Build(
      const boost::asio::ip::address& address, uint16_t port) override;

  std::unique_ptr<ConnectorInterface> Build(
      std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
      uint16_t port) override;

  std::unique_ptr<ConnectorInterface> Build(
      std::shared_ptr<utils::Domain> domain, uint16_t port) override;
};

class TcpConnector : public ConnectorInterface, private utils::LifeTime {
 public:
  using Factory = TcpConnectorFactory;

  TcpConnector(const boost::asio::ip::address& address, uint16_t port,
               boost::asio::io_service& io);

  TcpConnector(
      std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
      uint16_t port, boost::asio::io_service& io);

  TcpConnector(std::shared_ptr<utils::Domain> domain, uint16_t port,
               boost::asio::io_service& io);

  const utils::Cancelable& Connect(EventHandler handler) override;

  void Bind(std::shared_ptr<utils::DeviceInterface> device) override;

 private:
  void DoConnect(EventHandler handler);

  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses_;
  std::shared_ptr<utils::Domain> domain_;

  uint16_t port_;
  std::shared_ptr<utils::DeviceInterface> device_;
  utils::Cancelable resolve_cancelable_;

  std::error_code last_error_;

  std::size_t current_ind_{0};

  bool connecting_{false};
};

}  // namespace transport
}  // namespace nekit
