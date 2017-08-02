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

#include <functional>
#include <system_error>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "connection_interface.h"
#include "connector_interface.h"
#include "listener_interface.h"
#include "tcp_connector.h"
#include "tcp_listener.h"

namespace nekit {
namespace transport {

class TcpSocket final : public ConnectionInterface, private boost::noncopyable {
 public:
  ~TcpSocket() = default;

  void Read(std::unique_ptr<utils::Buffer>&&,
            TransportInterface::EventHandler) override;
  void Write(std::unique_ptr<utils::Buffer>&&,
             TransportInterface::EventHandler) override;

  void CloseRead() override;
  void CloseWrite() override;
  void Close() override;

  bool IsReadClosed() const override;
  bool IsWriteClosed() const override;
  bool IsClosed() const override;

  boost::asio::ip::tcp::endpoint localEndpoint() const override;
  boost::asio::ip::tcp::endpoint remoteEndpoint() const override;

  friend class TcpListener;
  friend class TcpConnector;

 private:
  TcpSocket(boost::asio::ip::tcp::socket&& socket);
  std::error_code ConvertBoostError(const boost::system::error_code&) const;

  boost::asio::ip::tcp::socket socket_;
  bool read_closed_{false}, write_closed_{false};
};
}  // namespace transport
}  // namespace nekit

