// MIT License

// Copyright (c) 2018 Zhuhao Wang

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

#include "nekit/data_flow/socks5_server_data_flow.h"

#include <boost/asio.hpp>
#include <boost/assert.hpp>

#include "nekit/utils/error.h"

namespace nekit {
namespace data_flow {
Socks5ServerDataFlow::Socks5ServerDataFlow(
    std::unique_ptr<LocalDataFlowInterface>&& data_flow,
    std::shared_ptr<utils::Session> session)
    : data_flow_{std::move(data_flow)}, session_{session} {
  BOOST_ASSERT_MSG(data_flow_->FlowDataType() == DataType::Stream,
                   "Packet type is not supported yet.");
}

const utils::Cancelable& Socks5ServerDataFlow::Read(
    std::unique_ptr<utils::Buffer>&& buffer, DataEventHandler handler) {
  read_cancelable_ = data_flow_->Read(
      std::move(buffer),
      [this, handler, cancelable{read_cancelable_},
       lifetime{life_time_cancelable_pointer()}](
          std::unique_ptr<utils::Buffer>&& buffer, std::error_code ec) {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        if (ec) {
          handler(nullptr, ec);
          return;
        }

        handler(std::move(buffer), ec);
      });

  return read_cancelable_;
}

const utils::Cancelable& Socks5ServerDataFlow::Write(
    std::unique_ptr<utils::Buffer>&& buffer, EventHandler handler) {
  write_cancelable_ = data_flow_->Write(
      std::move(buffer),
      [this, handler, cancelable{write_cancelable_},
       lifetime{life_time_cancelable_pointer()}](std::error_code ec) {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        handler(ec);
      });

  return write_cancelable_;
}

const utils::Cancelable& Socks5ServerDataFlow::CloseWrite(EventHandler handler) {
  return data_flow_->CloseWrite(handler);
}

bool Socks5ServerDataFlow::IsReadClosed() const {
  return reporting_ || data_flow_->IsReadClosed();
}

bool Socks5ServerDataFlow::IsWriteClosed() const {
  return reporting_ || data_flow_->IsWriteClosed();
}

bool Socks5ServerDataFlow::IsClosed() const {
  return reporting_ || data_flow_->IsClosed();
}

bool Socks5ServerDataFlow::IsReading() const {
  BOOST_ASSERT(forwarding_);
  return data_flow_->IsReading();
}

bool Socks5ServerDataFlow::IsWriting() const {
  BOOST_ASSERT(forwarding_);
  return data_flow_->IsWriting();
}

data_flow::DataFlowInterface* Socks5ServerDataFlow::NextHop() const {
  return data_flow_.get();
}

data_flow::DataType Socks5ServerDataFlow::FlowDataType() const {
  return DataType::Stream;
}

std::shared_ptr<utils::Session> Socks5ServerDataFlow::session() const {
  return session_;
}

boost::asio::io_context* Socks5ServerDataFlow::io() { return data_flow_->io(); }

const utils::Cancelable& Socks5ServerDataFlow::Open(EventHandler handler) {
  open_cancelable_ = utils::Cancelable();

  read_cancelable_ = data_flow_->Read(
      std::make_unique<utils::Buffer>(8),
      [this, handler, cancelable{open_cancelable_},
       lifetime{life_time_cancelable_pointer()}](
          std::unique_ptr<utils::Buffer> buffer, std::error_code ec) mutable {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        if (ec) {
          handler(ec);
          return;
        }

        if (buffer->size() <= 2) {
          handler(ErrorCode::RequestIncomplete);
          return;
        }

        if (buffer->GetByte(0) != 5) {
          handler(ErrorCode::UnsupportedVersion);
          return;
        }

        uint8_t len = buffer->GetByte(1);
        if (buffer->size() != 2 + len) {
          handler(ErrorCode::RequestIncomplete);
          return;
        }

        bool supported = false;
        while (len--) {
          // Only no auth is supported
          if (buffer->GetByte(2 + len) == 0) {
            supported = true;
            break;
          }
        }

        if (!supported) {
          handler(ErrorCode::UnsupportedAuthenticationMethod);
          return;
        }

        buffer->ShrinkBack(buffer->size() - 2);
        buffer->SetByte(1, 0);

        write_cancelable_ = data_flow_->Write(
            std::move(buffer), [this, handler, cancelable{open_cancelable_},
                                lifetime{life_time_cancelable_pointer()}](
                                   std::error_code ec) mutable {
              if (cancelable.canceled() || lifetime->canceled()) {
                return;
              }

              if (ec) {
                handler(ec);
                return;
              }

              read_cancelable_ = data_flow_->Read(
                  std::make_unique<utils::Buffer>(512),
                  [this, handler, cancelable{open_cancelable_},
                   lifetime{life_time_cancelable_pointer()}](
                      std::unique_ptr<utils::Buffer> buffer,
                      std::error_code ec) {
                    if (cancelable.canceled() || lifetime->canceled()) {
                      return;
                    }

                    if (ec) {
                      handler(ec);
                      return;
                    }

                    if (buffer->size() < 10) {
                      handler(ErrorCode::RequestIncomplete);
                      return;
                    }

                    if (buffer->GetByte(0) != 5) {
                      handler(ErrorCode::UnsupportedVersion);
                      return;
                    }

                    if (buffer->GetByte(1) != 1) {
                      handler(ErrorCode::UnsupportedCommand);
                      return;
                    }

                    size_t offset = 4;
                    switch (buffer->GetByte(2)) {
                      case 1: {
                        if (buffer->size() != 10) {
                          handler(ErrorCode::RequestIncomplete);
                          return;
                        }

                        auto bytes = boost::asio::ip::address_v4::bytes_type();
                        buffer->GetData(offset, bytes.size(), bytes.data());
                        session_->set_endpoint(
                            std::make_shared<utils::Endpoint>(
                                boost::asio::ip::address(
                                    boost::asio::ip::address_v4(bytes)),
                                0));
                        offset += bytes.size();

                      } break;
                      case 3: {
                        uint8_t len = buffer->GetByte(offset);
                        if (buffer->size() != offset + 1 + len + 2) {
                          handler(ErrorCode::RequestIncomplete);
                          return;
                        }

                        char* data = (char*)malloc(len);
                        if (!data) {
                          handler(
                              utils::NEKitErrorCode::MemoryAllocationFailed);
                          return;
                        }

                        buffer->GetData(offset + 1, len, data);
                        std::string host(data, len);

                        session_->set_endpoint(
                            std::make_shared<utils::Endpoint>(host, 0));

                        offset += (1 + len);
                      } break;
                      case 4: {
                        if (buffer->size() != 22) {
                          handler(ErrorCode::RequestIncomplete);
                          return;
                        }

                        auto bytes = boost::asio::ip::address_v6::bytes_type();
                        buffer->GetData(offset, bytes.size(), bytes.data());
                        session_->set_endpoint(
                            std::make_shared<utils::Endpoint>(
                                boost::asio::ip::address(
                                    boost::asio::ip::address_v6(bytes)),
                                0));
                        offset += bytes.size();
                      } break;
                    }

                    uint16_t port;
                    buffer->GetData(offset, 2, &port);
                    session_->endpoint()->set_port(ntohs(port));

                    handler(ErrorCode::NoError);
                  });

              cancelable.Dispose();
            });
        cancelable.Dispose();
      });

  return open_cancelable_;
}  // namespace data_flow

const utils::Cancelable& Socks5ServerDataFlow::Continue(EventHandler handler) {
  std::size_t len;
  uint8_t type;
  switch (session_->endpoint()->type()) {
    case utils::Endpoint::Type::Domain:
      len = 10;
      type = 1;
      break;
    case utils::Endpoint::Type::Address:
      if (session_->endpoint()->address().is_v4()) {
        len = 10;
        type = 1;
      } else {
        len = 22;
        type = 4;
      }
      break;
  }

  auto buffer = std::make_unique<utils::Buffer>(len);
  buffer->SetByte(0, 5);
  buffer->SetByte(1, 0);
  buffer->SetByte(2, 0);
  buffer->SetByte(3, type);

  for (size_t i = 4; i < len - 4; i++) {
    buffer->SetByte(i, 0);
  }

  write_cancelable_ = data_flow_->Write(
      std::move(buffer),
      // FIXME: wrong to capture write_cancelable here
      [this, handler, cancelable{write_cancelable_},
       lifetime{life_time_cancelable_pointer()}](std::error_code ec) mutable {
        if (cancelable.canceled() || lifetime->canceled()) {
          return;
        }

        handler(ec);
      });

  return write_cancelable_;
}

const utils::Cancelable& Socks5ServerDataFlow::ReportError(std::error_code error_code,
                                                     EventHandler handler) {
  (void)error_code;

  open_cancelable_ = utils::Cancelable();

  boost::asio::post(*io(), [this, handler, cancelable{open_cancelable_},
                            lifetime{life_time_cancelable_pointer()}]() {
    if (cancelable.canceled() || lifetime->canceled()) {
      return;
    }

    handler(ErrorCode::NoError);
  });
  return open_cancelable_;
}

LocalDataFlowInterface* Socks5ServerDataFlow::NextLocalHop() const {
  return data_flow_.get();
}

namespace {
struct Socks5ServerDataFlowErrorCategory : std::error_category {
  const char* name() const noexcept override;
  std::string message(int) const override;
};

const char* Socks5ServerDataFlowErrorCategory::name() const BOOST_NOEXCEPT {
  return "SOCKS5 server stream coder";
}

std::string Socks5ServerDataFlowErrorCategory::message(int error_code) const {
  switch (static_cast<Socks5ServerDataFlow::ErrorCode>(error_code)) {
    case Socks5ServerDataFlow::ErrorCode::NoError:
      return "no error";
    case Socks5ServerDataFlow::ErrorCode::RequestIncomplete:
      return "client send incomplete request";
    case Socks5ServerDataFlow::ErrorCode::UnsupportedAuthenticationMethod:
      return "all client requested authentication methods are not "
             "supported";
    case Socks5ServerDataFlow::ErrorCode::UnsupportedCommand:
      return "unknown command";
    case Socks5ServerDataFlow::ErrorCode::UnsupportedAddressType:
      return "unknown address type";
    case Socks5ServerDataFlow::ErrorCode::UnsupportedVersion:
      return "SOCKS version is not supported";
  }
}

const Socks5ServerDataFlowErrorCategory socks5ServerDataFlowErrorCategory{};

}  // namespace

std::error_code make_error_code(Socks5ServerDataFlow::ErrorCode ec) {
  return {static_cast<int>(ec), socks5ServerDataFlowErrorCategory};
}
}  // namespace data_flow
}  // namespace nekit
