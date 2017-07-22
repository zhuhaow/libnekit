#include <cassert>

#include "nekit/transport/tcp_socket.h"

namespace nekit {
namespace transport {

TcpSocket::TcpSocket(boost::asio::ip::tcp::socket &&socket)
    : socket_{std::move(socket)} {}

void TcpSocket::Read(std::unique_ptr<utils::Buffer> &&buffer,
                     TransportInterface::EventHandler &&handler) {
  read_buffer_ = std::move(buffer);
  read_handler_ = std::move(handler);
  socket_.async_read_some(
      boost::asio::mutable_buffers_1(buffer->buffer(), buffer->capacity()),
      [this](const boost::system::error_code &ec,
             std::size_t bytes_transferred) mutable {
        if (ec) {
          auto error = ConvertBoostError(ec);
          if (error == TcpSocketError::kEOF) {
            this->read_closed_ = true;
          }
          this->read_handler_(std::move(this->read_buffer_), error);

          this->read_buffer_ = nullptr;
          this->read_handler_ = TransportInterface::EventHandler();
          return;
        }

        this->read_buffer_->ReserveBack(this->read_buffer_->capacity() -
                                        bytes_transferred);
        this->read_handler_(std::move(this->read_buffer_),
                            TcpSocketError::kNoError);

        this->read_buffer_ = nullptr;
        this->read_handler_ = TransportInterface::EventHandler();
        return;
      });
};

void TcpSocket::Write(std::unique_ptr<utils::Buffer> &&buffer,
                      TransportInterface::EventHandler &&handler) {
  write_buffer_ = std::move(buffer);
  write_handler_ = std::move(handler);
  boost::asio::async_write(
      socket_,
      boost::asio::const_buffers_1(buffer->buffer(), buffer->capacity()),
      [this](const boost::system::error_code &ec,
             std::size_t bytes_transferred) mutable {
        assert(bytes_transferred == this->write_buffer_->capacity());

        if (ec) {
          this->write_handler_(std::move(this->write_buffer_),
                               ConvertBoostError(ec));

          this->write_buffer_ = nullptr;
          this->write_handler_ = TransportInterface::EventHandler();
          return;
        }

        this->write_handler_(std::move(this->write_buffer_),
                             TcpSocketError::kNoError);

        this->write_buffer_ = nullptr;
        this->write_handler_ = TransportInterface::EventHandler();
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

utils::Endpoint TcpSocket::localEndpoint() const {
  boost::system::error_code ec;
  utils::Endpoint endpoint = socket_.local_endpoint(ec);
  assert(!ec);
  return endpoint;
}

utils::Endpoint TcpSocket::remoteEndpoint() const {
  boost::system::error_code ec;
  utils::Endpoint endpoint = socket_.remote_endpoint(ec);
  assert(!ec);
  return endpoint;
}

TcpSocket::TcpSocketError TcpSocket::ConvertBoostError(
    const boost::system::error_code &ec) const {
  (void)ec;
  return TcpSocketError::kUnknownError;
}

namespace {
struct TcpSocketErrorCategory : std::error_category {
  const char *name() const noexcept override;
  std::string message(int) const override;
};

const char *TcpSocketErrorCategory::name() const noexcept {
  return "TCP socket";
}

std::string TcpSocketErrorCategory::message(int ev) const { return ""; }

const TcpSocketErrorCategory tcpSocketErrorCategory{};

}  // namespace

std::error_code make_error_code(TcpSocket::TcpSocketError e) {
  return {static_cast<int>(e), tcpSocketErrorCategory};
}
}  // namespace transport
}  // namespace nekit
