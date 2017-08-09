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
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "TCP Socket"

namespace nekit {
namespace transport {

TcpSocket::TcpSocket(boost::asio::ip::tcp::socket &&socket)
    : ConnectionInterface{socket.get_io_service()},
      socket_{std::move(socket)} {}

utils::Cancelable &TcpSocket::Read(std::unique_ptr<utils::Buffer> &&buffer,
                                   TransportInterface::EventHandler handler) {
  NETRACE << "Start reading data.";

  read_cancelable_ = utils::Cancelable();

  if (read_closed_) {
    NEERROR << "Socket reading is already closed. Do nothing.";
    return read_cancelable_;
  }

  socket_.async_read_some(
      boost::asio::mutable_buffers_1(buffer->buffer(), buffer->size()),
      [
        this, buffer{std::move(buffer)}, handler, cancelable{read_cancelable_},
        lifetime{life_time_cancelable_pointer()}
      ](const boost::system::error_code &ec,
        std::size_t bytes_transferred) mutable {

        if (cancelable.canceled() || lifetime->canceled() || IsReadClosed()) {
          return;
        }

        if (ec) {
          auto error = ConvertBoostError(ec);

          if (error == utils::NEKitErrorCode::Canceled) {
            return;
          }

          if (error == ErrorCode::EndOfFile) {
            this->read_closed_ = true;
            NEDEBUG << "Socket got EOF.";
          } else {
            NEERROR << "Reading from socket failed due to " << error << ".";
          }

          handler(std::move(buffer), error);
          return;
        }

        NETRACE << "Successfully read " << bytes_transferred
                << " bytes from socket.";
        buffer->ReserveBack(buffer->size() - bytes_transferred);
        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });
  return read_cancelable_;
}

utils::Cancelable &TcpSocket::Write(std::unique_ptr<utils::Buffer> &&buffer,
                                    TransportInterface::EventHandler handler) {
  NETRACE << "Start writing data.";

  write_cancelable_ = utils::Cancelable();

  if (write_closed_) {
    NEERROR << "Socket write is already closed. Do nothing.";
    return write_cancelable_;
  }

  boost::asio::async_write(
      socket_, boost::asio::const_buffers_1(buffer->buffer(), buffer->size()),
      [
        this, buffer{std::move(buffer)}, handler, cancelable{write_cancelable_},
        lifetime{life_time_cancelable_pointer()}
      ](const boost::system::error_code &ec,
        std::size_t bytes_transferred) mutable {

        if (cancelable.canceled() || lifetime->canceled() || IsWriteClosed()) {
          return;
        }

        if (ec) {
          auto error = ConvertBoostError(ec);
          NEERROR << "Write to socket failed due to " << error << ".";

          if (error == utils::NEKitErrorCode::Canceled) {
            return;
          }

          handler(std::move(buffer), error);
          return;
        }

        assert(bytes_transferred == buffer->size());

        NETRACE << "Successfully write " << bytes_transferred
                << " bytes to socket.";
        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });

  return write_cancelable_;
}

void TcpSocket::CloseRead() {
  NEDEBUG << "Closing socket reading.";
  if (read_closed_) {
    NEDEBUG << "Socket reading is already closed, nothing happened.";
    return;
  }

  boost::system::error_code ec;
  socket_.shutdown(socket_.shutdown_receive, ec);
  // It is not likely there will be any error other than those related to SSL.
  // But SSL is never used in TcpSocket.
  if (ec && ec != boost::asio::error::not_connected) {
    NEERROR << "Failed to close socket reading due to " << ec << ".";
    assert(!ec);
  }

  read_closed_ = true;
  NEDEBUG << "Socket reading closed.";
}

void TcpSocket::CloseWrite() {
  NEDEBUG << "Closing socket writing.";
  if (write_closed_) {
    NEDEBUG << "Socket writing is already closed, nothing happened.";
    return;
  }

  boost::system::error_code ec;
  socket_.shutdown(socket_.shutdown_send, ec);
  if (ec && ec != boost::asio::error::not_connected) {
    NEERROR << "Failed to close socket writing due to " << ec << ".";
    assert(!ec);
  }

  write_closed_ = true;
  NEDEBUG << "Socket writing closed.";
}

void TcpSocket::Close() {
  NEDEBUG << "Closing socket.";
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
      case boost::asio::error::basic_errors::operation_aborted:
        return nekit::utils::NEKitErrorCode::Canceled;
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
