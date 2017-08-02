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

#include <cassert>

#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/auto.h"
#include "nekit/utils/boost_error.h"

namespace nekit {
namespace transport {

TcpSocket::TcpSocket(boost::asio::ip::tcp::socket &&socket)
    : socket_{std::move(socket)} {}

void TcpSocket::Read(std::unique_ptr<utils::Buffer> &&buffer,
                     TransportInterface::EventHandler &&handler) {
  if (read_closed_) {
    socket_.get_io_service().post([
      buffer{std::move(buffer)}, handler{std::move(handler)}
    ]() mutable { handler(std::move(buffer), ErrorCode::Closed); });
    return;
  }

  socket_.async_read_some(
      boost::asio::mutable_buffers_1(buffer->buffer(), buffer->capacity()),
      [ this, buffer{std::move(buffer)}, handler{std::move(handler)} ](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {

        if (ec) {
          if (ec == boost::asio::error::basic_errors::operation_aborted) {
            // Cancel request must be from the host class, thus there is no need
            // to inform it.
            return;
          }

          auto error = ConvertBoostError(ec);
          if (error == ErrorCode::EndOfFile) {
            this->read_closed_ = true;
          }
          handler(std::move(buffer), error);
          return;
        }

        buffer->ReserveBack(buffer->capacity() - bytes_transferred);
        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });
};

void TcpSocket::Write(std::unique_ptr<utils::Buffer> &&buffer,
                      TransportInterface::EventHandler &&handler) {
  if (write_closed_) {
    socket_.get_io_service().post([
      buffer{std::move(buffer)}, handler{std::move(handler)}
    ]() mutable { handler(std::move(buffer), ErrorCode::Closed); });
    return;
  }

  boost::asio::async_write(
      socket_,
      boost::asio::const_buffers_1(buffer->buffer(), buffer->capacity()),
      [ this, buffer{std::move(buffer)}, handler{std::move(handler)} ](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {
        assert(bytes_transferred == buffer->capacity());

        if (ec) {
          if (ec.value() ==
              boost::asio::error::basic_errors::operation_aborted) {
            return;
          }

          handler(std::move(buffer), ConvertBoostError(ec));
          return;
        }

        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });
}

void TcpSocket::CloseRead() {
  if (read_closed_) return;

  boost::system::error_code ec;
  socket_.shutdown(socket_.shutdown_receive, ec);
  // It is not likely there will be any error other than those related to SSL.
  // But SSL is never used in TcpSocket.
  assert(!ec);
  read_closed_ = true;
}

void TcpSocket::CloseWrite() {
  if (write_closed_) return;

  boost::system::error_code ec;
  socket_.shutdown(socket_.shutdown_send, ec);
  assert(!ec);
  write_closed_ = true;
}

void TcpSocket::Close() {
  CloseRead();
  CloseWrite();
}

bool TcpSocket::IsReadClosed() const { return read_closed_; }

bool TcpSocket::IsWriteClosed() const { return write_closed_; }

bool TcpSocket::IsClosed() const { return IsReadClosed() && IsWriteClosed(); }

boost::asio::ip::tcp::endpoint TcpSocket::localEndpoint() const {
  boost::system::error_code ec;
  boost::asio::ip::tcp::endpoint endpoint = socket_.local_endpoint(ec);
  assert(!ec);
  return endpoint;
}

boost::asio::ip::tcp::endpoint TcpSocket::remoteEndpoint() const {
  boost::system::error_code ec;
  boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(ec);
  assert(!ec);
  return endpoint;
}

std::error_code TcpSocket::ConvertBoostError(
    const boost::system::error_code &ec) const {
  if (ec.category() == boost::asio::error::system_category) {
    switch (ec.value()) {
      case boost::asio::error::basic_errors::connection_aborted:
        return ErrorCode::ConnectionAborted;
      case boost::asio::error::basic_errors::connection_reset:
        return ErrorCode::ConnectionReset;
      case boost::asio::error::basic_errors::host_unreachable:
        return ErrorCode::HostUnreachable;
      case boost::asio::error::basic_errors::network_down:
        return ErrorCode::NetworkDown;
      case boost::asio::error::basic_errors::network_reset:
        return ErrorCode::NetworkReset;
      case boost::asio::error::basic_errors::network_unreachable:
        return ErrorCode::NetworkUnreachable;
      case boost::asio::error::basic_errors::timed_out:
        return ErrorCode::TimedOut;
    }
  } else if (ec.category() == boost::asio::error::misc_category) {
    if (ec.value() == boost::asio::error::misc_errors::eof) {
      return ErrorCode::EndOfFile;
    }
  }

  // Semantically we should return UnknownError, but ideally, UnknownError
  // should never occur, we should treat every error carefully, and this
  // provides us the necessary information to handle the origin error.
  return std::make_error_code(ec);
}
}  // namespace transport
}  // namespace nekit
