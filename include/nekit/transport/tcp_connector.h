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

#include "../utils/cancelable.h"
#include "../utils/device.h"
#include "../utils/endpoint.h"

namespace nekit {
namespace transport {

class TcpConnector : public utils::AsyncIoInterface, private utils::LifeTime {
 public:
  using EventHandler =
      std::function<void(boost::asio::ip::tcp::socket&&, std::error_code)>;

  TcpConnector(const boost::asio::ip::address& address, uint16_t port,
               boost::asio::io_context* io);

  TcpConnector(
      std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
      uint16_t port, boost::asio::io_context* io);

  TcpConnector(std::shared_ptr<utils::Endpoint> endpoint,
               boost::asio::io_context* io);

  utils::Cancelable Connect(EventHandler handler)
      __attribute__((warn_unused_result));

  void Bind(std::shared_ptr<utils::DeviceInterface> device);

  boost::asio::io_context* io() override;

 private:
  void DoConnect(EventHandler handler);

  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses_;
  boost::asio::ip::address address_;
  std::shared_ptr<utils::Endpoint> endpoint_;

  uint16_t port_;
  std::shared_ptr<utils::DeviceInterface> device_;

  std::error_code last_error_;

  std::size_t current_ind_{0};

  bool connecting_{false};
};

}  // namespace transport
}  // namespace nekit
