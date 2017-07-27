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

#define BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS

#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/auto.h"
#include "nekit/utils/boost_error.h"

namespace nekit {
namespace transport {

TcpSocket::TcpSocket(boost::asio::ip::tcp::socket &&socket)
    : socket_{std::move(socket)} {}

void TcpSocket::Read(std::unique_ptr<utils::Buffer> &&buffer,
                     TransportInterface::EventHandler &&handler) {
  // read_buffer_ = std::move(buffer);
  // read_handler_ = std::move(handler);
  socket_.async_read_some(
      boost::asio::mutable_buffers_1(buffer->buffer(), buffer->capacity()),
      [ this, buffer{std::move(buffer)}, handler{std::move(handler)} ](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {

        if (ec) {
          if (ec.category() == boost::asio::error::system_category &&
              ec.value() ==
                  boost::asio::error::basic_errors::operation_aborted) {
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
  boost::asio::async_write(
      socket_,
      boost::asio::const_buffers_1(buffer->buffer(), buffer->capacity()),
      [ this, buffer{std::move(buffer)}, handler{std::move(handler)} ](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {
        assert(bytes_transferred == buffer->capacity());

        if (ec) {
          if (ec.category() == boost::asio::error::system_category &&
              ec.value() ==
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
  return std::make_error_code(ec);
}  // namespace transport

namespace {
struct TcpSocketErrorCategory : std::error_category {
  const char *name() const noexcept override;
  std::string message(int) const override;
};

const char *TcpSocketErrorCategory::name() const noexcept {
  return "TCP socket";
}

std::string TcpSocketErrorCategory::message(int ev) const {
  switch (static_cast<TcpSocket::ErrorCode>(ev)) {
    case TcpSocket::ErrorCode::NoError:
      return "no error";
    case TcpSocket::ErrorCode::ConnectionAborted:
      return "connection aborted";
    case TcpSocket::ErrorCode::ConnectionReset:
      return "connection reset";
    case TcpSocket::ErrorCode::HostUnreachable:
      return "host unreachable";
    case TcpSocket::ErrorCode::NetworkDown:
      return "network down";
    case TcpSocket::ErrorCode::NetworkReset:
      return "network reset";
    case TcpSocket::ErrorCode::NetworkUnreachable:
      return "network unreachable";
    case TcpSocket::ErrorCode::TimedOut:
      return "timeout";
    case TcpSocket::ErrorCode::EndOfFile:
      return "end of file";
    case TcpSocket::ErrorCode::UnknownError:
      return "unknown error";
  }
}

const TcpSocketErrorCategory tcpSocketErrorCategory{};

}  // namespace

std::error_code make_error_code(TcpSocket::ErrorCode e) {
  return {static_cast<int>(e), tcpSocketErrorCategory};
}
}  // namespace transport
}  // namespace nekit
