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
          std::make_unique<std::vector<boost::asio::mutable_buffer>>(0)} {
  BOOST_ASSERT(&socket_.get_io_context() == session->io());
}

TcpSocket::TcpSocket(std::shared_ptr<utils::Session> session)
    : socket_{*session->io()},
      session_{session},
      connect_to_{session->current_endpoint()},
      write_buffer_{
          std::make_unique<std::vector<boost::asio::const_buffer>>(0)},
      read_buffer_{
          std::make_unique<std::vector<boost::asio::mutable_buffer>>(0)} {}

const utils::Cancelable &TcpSocket::Read(
    std::unique_ptr<utils::Buffer> &&buffer, DataEventHandler handler) {
  BOOST_ASSERT(ready_);
  BOOST_ASSERT(socket_.is_open());
  BOOST_ASSERT(!read_closed_);
  BOOST_ASSERT(!reading_);
  BOOST_ASSERT(!errored_);
  BOOST_ASSERT(buffer->size());
  BOOST_ASSERT(read_buffer_ && !read_buffer_->size());

  NETRACE << "Start reading data.";

  read_cancelable_ = utils::Cancelable();

  buffer->WalkInternalChunk(
      [this](void *d, size_t s, void *c) {
        (void)c;
        read_buffer_->push_back(boost::asio::mutable_buffer(d, s));
        return true;
      },
      0, nullptr);

  reading_ = true;

  socket_.async_read_some(
      *read_buffer_,
      [this, buffer{std::move(buffer)}, buffer_wrapper{std::move(read_buffer_)},
       handler, cancelable{read_cancelable_},
       lifetime{life_time_cancelable_pointer()}](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        reading_ = false;

        if (ec) {
          auto error = ConvertBoostError(ec);

          if (error == utils::NEKitErrorCode::Canceled) {
            return;
          }

          if (error == ErrorCode::EndOfFile) {
            read_closed_ = true;
            NEDEBUG << "Socket got EOF.";
          } else {
            NEERROR << "Reading from socket failed due to " << error << ".";
            errored_ = true;
            read_closed_ = true;
          }

          handler(std::move(buffer), error);
          return;
        }

        NETRACE << "Successfully read " << bytes_transferred
                << " bytes from socket.";

        if (bytes_transferred != buffer->size()) {
          buffer->ShrinkBack(buffer->size() - bytes_transferred);
        }

        read_buffer_ = std::move(buffer_wrapper);
        read_buffer_->clear();

        handler(std::move(buffer), ErrorCode::NoError);
        return;
      });
  return read_cancelable_;
}

const utils::Cancelable &TcpSocket::Write(
    std::unique_ptr<utils::Buffer> &&buffer, EventHandler handler) {
  BOOST_ASSERT(ready_);
  BOOST_ASSERT(socket_.is_open());
  BOOST_ASSERT(!write_closed_);
  BOOST_ASSERT(!writing_);
  BOOST_ASSERT(!errored_);
  BOOST_ASSERT(buffer->size());
  BOOST_ASSERT(write_buffer_ && !write_buffer_->size());

  NETRACE << "Start writing data.";

  write_cancelable_ = utils::Cancelable();

  buffer->WalkInternalChunk(
      [this](void *d, size_t s, void *c) {
        (void)c;
        write_buffer_->push_back(boost::asio::const_buffer(d, s));
        return false;
      },
      0, nullptr);

  writing_ = true;

  boost::asio::async_write(
      socket_, *write_buffer_,
      [this, buffer{std::move(buffer)},
       buffer_wrapper{std::move(write_buffer_)}, handler,
       cancelable{write_cancelable_}, lifetime{life_time_cancelable_pointer()}](
          const boost::system::error_code &ec,
          std::size_t bytes_transferred) mutable {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        writing_ = false;

        if (ec) {
          auto error = ConvertBoostError(ec);
          NEERROR << "Write to socket failed due to " << error << ".";

          if (error == utils::NEKitErrorCode::Canceled) {
            return;
          }

          errored_ = true;

          handler(error);
          return;
        }

        NETRACE << "Successfully write " << bytes_transferred
                << " bytes to socket.";

        BOOST_ASSERT(bytes_transferred == buffer->size());

        handler(ErrorCode::NoError);
        return;
      });
  return write_cancelable_;
}

const utils::Cancelable &TcpSocket::CloseWrite(EventHandler handler) {
  BOOST_ASSERT(ready_);
  BOOST_ASSERT(!writing_);

  NEDEBUG << "Closing socket writing.";

  std::error_code error;

  if (write_closed_) {
    NEDEBUG << "Socket writing is already closed, nothing happened.";
    error = ErrorCode::NoError;
  } else {
    boost::system::error_code ec;
    socket_.shutdown(socket_.shutdown_send, ec);
    if (ec && ec != boost::asio::error::not_connected) {
      NEERROR << "Failed to close socket writing due to " << ec << ".";
    }

    write_closed_ = true;
    NEDEBUG << "Socket writing closed.";

    error = ConvertBoostError(ec);
  }

  write_cancelable_ = utils::Cancelable();
  writing_ = true;  // Probably not necessary, just guard some strange usage.

  boost::asio::post(*io(), [this, handler, cancelable{write_cancelable_},
                            lifetime{life_time_cancelable_pointer()}, error]() {
    if (cancelable.canceled() || lifetime->canceled()) {
      return;
    }

    writing_ = false;

    handler(error);
  });
  return write_cancelable_;
}

bool TcpSocket::IsReadClosed() const {
  BOOST_ASSERT(ready_);
  return read_closed_;
}

bool TcpSocket::IsWriteClosed() const {
  BOOST_ASSERT(ready_);
  return write_closed_;
}

bool TcpSocket::IsClosed() const {
  BOOST_ASSERT(ready_);
  return IsReadClosed() && IsWriteClosed();
}

bool TcpSocket::IsReading() const {
  BOOST_ASSERT(ready_);
  return reading_;
}

bool TcpSocket::IsWriting() const {
  BOOST_ASSERT(ready_);
  return writing_;
}

bool TcpSocket::IsReady() const { return ready_; }

bool TcpSocket::IsStopped() const {
  return ready_ ? !writing_ && !reading_ && !processing_ : !processing_;
}

data_flow::DataFlowInterface *TcpSocket::NextHop() const { return nullptr; }

std::shared_ptr<utils::Endpoint> TcpSocket::ConnectingTo() {
  return connect_to_;
}

data_flow::DataType TcpSocket::FlowDataType() const {
  return data_flow::DataType::Stream;
}

std::shared_ptr<utils::Session> TcpSocket::session() const { return session_; }

boost::asio::io_context *TcpSocket::io() { return &socket_.get_io_context(); }

const utils::Cancelable &TcpSocket::Open(EventHandler handler) {
  BOOST_ASSERT(!ready_);
  BOOST_ASSERT(socket_.is_open());
  BOOST_ASSERT(!processing_);

  report_cancelable_ =
      utils::Cancelable();  // Report error request will invalid this callback,
                            // so we just use report cancelable to guard the
                            // lifetime here then there is no need to cancel it
                            // explicitly when report error is requested.
  processing_ = true;

  boost::asio::post(*io(), [this, handler, cancelable{report_cancelable_},
                            lifetime{life_time_cancelable_pointer()}]() {
    if (cancelable.canceled() || lifetime->canceled()) {
      return;
    }

    processing_ = false;

    handler(ErrorCode::NoError);
  });
  return report_cancelable_;
}

const utils::Cancelable &TcpSocket::Continue(EventHandler handler) {
  BOOST_ASSERT(!ready_);
  BOOST_ASSERT(socket_.is_open());
  BOOST_ASSERT(!processing_);

  report_cancelable_ = utils::Cancelable();

  processing_ = true;

  boost::asio::post(*io(), [this, handler, cancelable{report_cancelable_},
                            lifetime{life_time_cancelable_pointer()}]() {
    if (cancelable.canceled() || lifetime->canceled()) {
      return;
    }

    ready_ = true;
    processing_ = false;

    handler(ErrorCode::NoError);
  });
  return report_cancelable_;
}

const utils::Cancelable &TcpSocket::ReportError(std::error_code ec,
                                                EventHandler handler) {
  BOOST_ASSERT(errored_);

  (void)ec;

  read_cancelable_.Dispose();
  write_cancelable_.Dispose();
  connect_cancelable_.Dispose();

  reading_ = false;
  writing_ = false;
  ready_ = false;
  read_closed_ = true;
  write_closed_ = true;

  report_cancelable_ = utils::Cancelable();
  processing_ = true;

  boost::asio::post(*io(), [this, handler, cancelable{report_cancelable_},
                            lifetime{life_time_cancelable_pointer()}]() {
    if (cancelable.canceled() || lifetime->canceled()) {
      return;
    }

    processing_ = false;

    handler(ErrorCode::NoError);
  });
  return report_cancelable_;
}

data_flow::LocalDataFlowInterface *TcpSocket::NextLocalHop() const {
  return nullptr;
}

const utils::Cancelable &TcpSocket::Connect(EventHandler handler) {
  BOOST_ASSERT(!ready_);
  BOOST_ASSERT(!processing_);
  BOOST_ASSERT(connect_to_);

  connector_ = std::make_unique<TcpConnector>(connect_to_, io());

  processing_ = true;

  connect_cancelable_ = connector_->Connect(
      [this, handler, cancelable{connect_cancelable_}](
          boost::asio::ip::tcp::socket &&socket, std::error_code ec) {
        if (cancelable.canceled()) {
          return;
        }

        processing_ = false;
        ready_ = true;

        if (ec) {
          handler(ec);
          return;
        }

        socket_ = std::move(socket);
        handler(ec);
        return;
      });

  return connect_cancelable_;
}

data_flow::RemoteDataFlowInterface *TcpSocket::NextRemoteHop() const {
  return nullptr;
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
