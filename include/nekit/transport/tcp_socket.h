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

namespace nekit {
namespace transport {

class TcpSocket final : public ConnectionInterface, private boost::noncopyable {
 public:
  enum class ErrorCode {
    NoError = 0,
    ConnectionAborted,
    ConnectionReset,
    HostUnreachable,
    NetworkDown,
    NetworkReset,
    NetworkUnreachable,
    TimedOut,
    EndOfFile,
    UnknownError
  };

  ~TcpSocket() = default;

  void Read(std::unique_ptr<utils::Buffer>&&,
            TransportInterface::EventHandler&&) override;
  void Write(std::unique_ptr<utils::Buffer>&&,
             TransportInterface::EventHandler&&) override;

  void CloseRead() override;
  void CloseWrite() override;
  void Close() override;

  bool IsReadClosed() const override;
  bool IsWriteClosed() const override;
  bool IsClosed() const override;

  utils::Endpoint localEndpoint() const override;
  utils::Endpoint remoteEndpoint() const override;

 private:
  TcpSocket(boost::asio::ip::tcp::socket&& socket);
  ErrorCode ConvertBoostError(const boost::system::error_code&) const;

  boost::asio::ip::tcp::socket socket_;
  bool read_closed_{false}, write_closed_{false};

  // Due to boost limit, the callback handler must be copyable, thus we have to
  // keep them here.
  std::unique_ptr<utils::Buffer> read_buffer_, write_buffer_;
  TransportInterface::EventHandler read_handler_, write_handler_;
};

std::error_code make_error_code(TcpSocket::ErrorCode e);

}  // namespace transport
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::transport::TcpSocket::ErrorCode> : true_type {
};
}  // namespace std
