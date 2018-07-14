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

#include <boost/assert.hpp>

#include "nekit/transport/error_code.h"
#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/auto.h"
#include "nekit/utils/boost_error.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "TCP Socket"

namespace nekit {
namespace transport {

TcpSocket::TcpSocket(boost::asio::ip::tcp::socket &&socket,
                     std::shared_ptr<utils::Session> session)
    : socket_{std::move(socket)},
      session_{session},
      write_buffer_{
          std::make_unique<std::vector<boost::asio::const_buffer>>(0)},
      read_buffer_{
          std::make_unique<std::vector<boost::asio::mutable_buffer>>(0)},
      state_machine_{data_flow::FlowType::Local} {
  BOOST_ASSERT(&socket_.get_io_context() == session->io());
}

TcpSocket::TcpSocket(std::shared_ptr<utils::Session> session)
    : socket_{*session->io()},
      session_{session},
      connect_to_{session->current_endpoint()},
      write_buffer_{
          std::make_unique<std::vector<boost::asio::const_buffer>>(0)},
      read_buffer_{
          std::make_unique<std::vector<boost::asio::mutable_buffer>>(0)},
      state_machine_{data_flow::FlowType::Remote} {}

TcpSocket::~TcpSocket() {
  read_cancelable_.Cancel();
  write_cancelable_.Cancel();
  report_cancelable_.Cancel();
  connect_cancelable_.Cancel();
}

utils::Cancelable TcpSocket::Read(utils::Buffer &&buffer,
                                  DataEventHandler handler) {
  NETRACE << "Start reading data.";

  read_cancelable_ = utils::Cancelable();

  buffer.WalkInternalChunk(
      [this](void *d, size_t s, void *c) {
        (void)c;
        read_buffer_->push_back(boost::asio::mutable_buffer(d, s));
        return true;
      },
      0, nullptr);

  state_machine_.ReadBegin();
  socket_.async_read_some(
      *read_buffer_,
      [this, buffer{std::move(buffer)}, buffer_wrapper{std::move(read_buffer_)},
       handler,
       cancelable{read_cancelable_}](const boost::system::error_code &ec,
                                     std::size_t bytes_transferred) mutable {
        if (cancelable.canceled()) {
          return;
        }

        state_machine_.ReadEnd();

        if (ec) {
          auto error = ConvertBoostError(ec);

          if (error == ErrorCode::EndOfFile) {
            state_machine_.ReadClosed();
            NEDEBUG << "Socket got EOF.";
          } else {
            NEERROR << "Reading from socket failed due to " << error << ".";
            state_machine_.Errored();
            // report and connect cancelable should not be in use.
            write_cancelable_.Cancel();
          }

          handler(std::move(buffer), error);
          return;
        }

        NETRACE << "Successfully read " << bytes_transferred
                << " bytes from socket.";

        if (bytes_transferred != buffer.size()) {
          buffer.ShrinkBack(buffer.size() - bytes_transferred);
        }

        read_buffer_ = std::move(buffer_wrapper);
        read_buffer_->clear();

        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });
  return read_cancelable_;
}

utils::Cancelable TcpSocket::Write(utils::Buffer &&buffer,
                                   EventHandler handler) {
  NETRACE << "Start writing data.";

  write_cancelable_ = utils::Cancelable();

  buffer.WalkInternalChunk(
      [this](void *d, size_t s, void *c) {
        (void)c;
        write_buffer_->emplace_back(d, s);
        return true;
      },
      0, nullptr);

  state_machine_.WriteBegin();

  boost::asio::async_write(
      socket_, *write_buffer_,
      [this, buffer{std::move(buffer)}, handler, cancelable{write_cancelable_}](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {
        if (cancelable.canceled()) {
          return;
        }

        state_machine_.WriteEnd();

        if (ec) {
          auto error = ConvertBoostError(ec);
          NEERROR << "Write to socket failed due to " << error << ".";

          state_machine_.Errored();
          // report and connect cancelable should not be in use.
          read_cancelable_.Cancel();

          handler(error);
          return;
        }

        NETRACE << "Successfully write " << bytes_transferred
                << " bytes to socket.";

        BOOST_ASSERT(bytes_transferred == buffer.size());

        write_buffer_->clear();

        handler(ErrorCode::NoError);
        return;
      });
  return write_cancelable_;
}

utils::Cancelable TcpSocket::CloseWrite(EventHandler handler) {
  NEDEBUG << "Closing socket writing.";

  std::error_code error;

  boost::system::error_code ec;
  socket_.shutdown(socket_.shutdown_send, ec);
  if (ec && ec != boost::asio::error::not_connected) {
    NEERROR << "Failed to close socket writing due to " << ec << ".";
  }

  NEDEBUG << "Socket writing closed.";

  error = ConvertBoostError(ec);

  write_cancelable_ = utils::Cancelable();

  state_machine_.WriteCloseBegin();

  boost::asio::post(*io(),
                    [this, handler, cancelable{write_cancelable_}, error]() {
                      if (cancelable.canceled()) {
                        return;
                      }

                      state_machine_.WriteCloseEnd();

                      handler(error);
                    });

  return write_cancelable_;
}

const data_flow::FlowStateMachine &TcpSocket::StateMachine() const {
  return state_machine_;
}

data_flow::DataFlowInterface *TcpSocket::NextHop() const { return nullptr; }

std::shared_ptr<utils::Endpoint> TcpSocket::ConnectingTo() {
  return connect_to_;
}

data_flow::DataType TcpSocket::FlowDataType() const {
  return data_flow::DataType::Stream;
}

std::shared_ptr<utils::Session> TcpSocket::Session() const { return session_; }

boost::asio::io_context *TcpSocket::io() { return &socket_.get_io_context(); }

utils::Cancelable TcpSocket::Open(EventHandler handler) {
  state_machine_.ConnectBegin();

  report_cancelable_ =
      utils::Cancelable();  // Report error request will invalid this callback,
                            // so we just use report cancelable to guard the
                            // lifetime here.

  NETRACE << "Open TCP socket that is already connected, do nothing.";

  boost::asio::post(*io(), [handler, cancelable{report_cancelable_}]() {
    if (cancelable.canceled()) {
      return;
    }

    NETRACE << "Opened callback called.";

    handler(ErrorCode::NoError);
  });

  return report_cancelable_;
}

utils::Cancelable TcpSocket::Continue(EventHandler handler) {
  report_cancelable_ = utils::Cancelable();

  NETRACE << "Continue to establish connection.";

  boost::asio::post(*io(), [this, handler, cancelable{report_cancelable_}]() {
    if (cancelable.canceled()) {
      return;
    }

    state_machine_.Connected();

    NETRACE << "Connection is established.";

    handler(ErrorCode::NoError);
  });
  return report_cancelable_;
}

utils::Cancelable TcpSocket::Connect(EventHandler handler) {
  connect_to_ = session_->current_endpoint();

  BOOST_ASSERT(connect_to_);

  connector_ = std::make_unique<TcpConnector>(connect_to_, io());

  state_machine_.ConnectBegin();

  connect_cancelable_ = connector_->Connect(
      [this, handler, cancelable{connect_cancelable_}](
          boost::asio::ip::tcp::socket &&socket, std::error_code ec) {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          state_machine_.Errored();
          handler(ec);
          return;
        }

        state_machine_.Connected();
        socket_ = std::move(socket);
        handler(ec);
        return;
      });

  return connect_cancelable_;
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
