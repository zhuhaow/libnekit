#include "nekit/utils/boost_tcp_socket.h"

namespace nekit {
namespace utils {

BoostTcpSocket::BoostTcpSocket(boost::asio::io_service &service)
    : BoostTcpSocket(service, nullptr) {}

BoostTcpSocket::BoostTcpSocket(boost::asio::io_service &service,
                               DelegatePointer delegate)
    : socket_(service),
      delegate_(delegate),
      connected_(false),
      eof_(false),
      shutdown_(false),
      closed_(false) {}

BoostTcpSocket::Pointer BoostTcpSocket::CreateSocket(
    boost::asio::io_service &service) {
  // Not using `make_shared` since the ctor is private. This class if final thus
  // it is not able to workaround it. Anyway, considering this is a heavy class,
  // the overhead is negligible.
  return std::shared_ptr<BoostTcpSocket>(new BoostTcpSocket(service));
}

BoostTcpSocket::Pointer BoostTcpSocket::CreateSocket(
    boost::asio::io_service &service, DelegatePointer delegate) {
  return std::shared_ptr<BoostTcpSocket>(new BoostTcpSocket(service, delegate));
}

void BoostTcpSocket::set_delegate(DelegatePointer delegate) {
  delegate_ = delegate;
}

void BoostTcpSocket::Connect(const boost::asio::ip::tcp::endpoint endpoint) {
  auto self(this->shared_from_this());
  socket_.async_connect(endpoint,
                        [this, self](const boost::system::error_code &ec) {
                          if (ec) {
                            HandleError(ec);
                            return;
                          }
                          if (delegate_)
                            delegate_->DidConnect(this->shared_from_this());
                        });
}

Error BoostTcpSocket::Bind(const boost::asio::ip::tcp::endpoint endpoint) {
  boost::system::error_code ec;
  socket_.bind(endpoint, ec);
  return ConvertBoostError(ec);
}

Error BoostTcpSocket::Shutdown() {
  boost::system::error_code ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
  shutdown_ = true;
  CheckClose();
  return ConvertBoostError(ec);
}

void BoostTcpSocket::Read(std::shared_ptr<Buffer> buffer) {
  auto self(this->shared_from_this());
  socket_.async_read_some(
      boost::asio::mutable_buffers_1(buffer->buffer(), buffer->capacity()),
      [this, self, buffer](const boost::system::error_code &ec,
                           std::size_t bytes_transferred) {
        if (ec) {
          HandleError(ec);
          return;
        }

        if (delegate_) {
          buffer->ReserveBack(buffer->capacity() - bytes_transferred);
          delegate_->DidRead(buffer, self);
          return;
        }
      });
}

void BoostTcpSocket::Write(const std::shared_ptr<Buffer> buffer) {
  auto self(this->shared_from_this());
  socket_.async_send(
      boost::asio::const_buffers_1(buffer->buffer(), buffer->capacity()),
      [this, self, buffer](const boost::system::error_code &ec,
                           std::size_t bytes_transferred) {
        (void)bytes_transferred;

        if (ec) {
          HandleError(ec);
          return;
        }

        if (delegate_) {
          delegate_->DidWrite(buffer, self);
          return;
        }
      });
}

void BoostTcpSocket::HandleError(const boost::system::error_code &ec) {
  if (ec.category() == boost::asio::error::get_system_category()) {
    if (ec.value() == boost::asio::error::operation_aborted) {
      return;
    }
  } else if (ec.category() == boost::asio::error::get_misc_category()) {
    if (ec.value() == boost::asio::error::eof) {
      if (delegate_) {
        eof_ = true;
        delegate_->DidEOF(this->shared_from_this());
        CheckClose();
        return;
      }
    }
  }

  if (delegate_) {
    delegate_->DidError(ConvertBoostError(ec), this->shared_from_this());
    return;
  }
}

Error BoostTcpSocket::ConvertBoostError(const boost::system::error_code &ec) {
  (void)ec;
  // TODO: Convert boost error to custom tcp error.
  return std::make_error_code(TcpSocketInterface::kUnknownError);
}

void BoostTcpSocket::CheckClose() {
  if (!closed_ && connected_ && eof_ && shutdown_) {
    closed_ = true;
    auto self(this->shared_from_this());
    socket_.get_io_service().post([this, self]() {
      if (this->delegate_) {
        this->delegate_->DidClose(self);
      }
    });
  }
}

}  // namespace utils
}  // namespace nekit
