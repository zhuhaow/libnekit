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

#include "nekit/transport/error_code.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#define AUTH_READ_BUFFER_SIZE 512

#undef NECHANNEL
#define NECHANNEL "SOCKS5 Server"

namespace nekit {
namespace data_flow {
Socks5ServerDataFlow::Socks5ServerDataFlow(
    std::unique_ptr<LocalDataFlowInterface>&& data_flow,
    std::shared_ptr<utils::Session> session)
    : data_flow_{std::move(data_flow)}, session_{session} {
  BOOST_ASSERT_MSG(data_flow_->FlowDataType() == DataType::Stream,
                   "Packet type is not supported yet.");
}

Socks5ServerDataFlow::~Socks5ServerDataFlow() {
  open_cancelable_.Cancel();
  read_cancelable_.Cancel();
  write_cancelable_.Cancel();
}

utils::Cancelable Socks5ServerDataFlow::Read(utils::Buffer&& buffer,
                                             DataEventHandler handler) {
  BOOST_ASSERT(!reading_);
  BOOST_ASSERT(!reporting_);
  BOOST_ASSERT(!read_closed_);
  BOOST_ASSERT(!NextHop()->IsReading());
  BOOST_ASSERT(state_ != data_flow::State::Closed);

  reading_ = true;

  read_cancelable_ = data_flow_->Read(
      std::move(buffer),
      [this, handler](utils::Buffer&& buffer, std::error_code ec) {
        reading_ = false;

        if (ec) {
          if (ec == transport::ErrorCode::EndOfFile) {
            read_closed_ = true;
            if (write_closed_ && !writing_) {
              // Write is already closed.
              state_ = data_flow::State::Closed;
            } else {
              state_ = data_flow::State::Closing;
            }
            NEDEBUG << "Data flow got EOF.";
          } else {
            NEERROR << "Reading from data flow failed due to " << ec << ".";
            state_ = data_flow::State::Closed;
            read_closed_ = true;
            write_closed_ = true;
            // report and connect cancelable should not be in use.
            write_cancelable_.Cancel();
          }
          handler(std::move(buffer), ec);
          return;
        }

        handler(std::move(buffer), ec);
      });

  return read_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::Write(utils::Buffer&& buffer,
                                              EventHandler handler) {
  BOOST_ASSERT(!writing_);
  BOOST_ASSERT(!reporting_);
  BOOST_ASSERT(!write_closed_);
  BOOST_ASSERT(!NextHop()->IsWriting());
  BOOST_ASSERT(state_ != data_flow::State::Closed);

  writing_ = true;

  write_cancelable_ =
      data_flow_->Write(std::move(buffer), [this, handler](std::error_code ec) {
        writing_ = false;

        if (ec) {
          NEERROR << "Write to data flow failed due to " << ec << ".";

          read_closed_ = true;
          write_closed_ = true;
          state_ = data_flow::State::Closed;
          // report and connect cancelable should not be in use.
          read_cancelable_.Cancel();
        }
        handler(ec);
      });

  return write_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::CloseWrite(EventHandler handler) {
  BOOST_ASSERT(!writing_);
  BOOST_ASSERT(!reporting_);
  BOOST_ASSERT(!write_closed_);
  BOOST_ASSERT(!NextHop()->IsWriting());

  writing_ = true;
  write_closed_ = true;

  state_ = data_flow::State::Closing;

  write_cancelable_ =
      data_flow_->CloseWrite([this, handler](std::error_code ec) {
        writing_ = false;

        if (read_closed_) {
          state_ = data_flow::State::Closed;
        }

        handler(ec);
      });

  return write_cancelable_;
}

bool Socks5ServerDataFlow::IsReadClosed() const {
  NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_);
  return read_closed_;
}

bool Socks5ServerDataFlow::IsWriteClosed() const {
  NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_);
  return write_closed_;
}

bool Socks5ServerDataFlow::IsWriteClosing() const {
  NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_);
  return write_closed_ && writing_;
}

bool Socks5ServerDataFlow::IsReading() const {
  NE_DATA_FLOW_CAN_CHECK_DATA_STATE(state_);
  return data_flow_->IsReading();
}

bool Socks5ServerDataFlow::IsWriting() const {
  NE_DATA_FLOW_CAN_CHECK_DATA_STATE(state_);
  return data_flow_->IsWriting();
}

data_flow::State Socks5ServerDataFlow::State() const { return state_; }

data_flow::DataFlowInterface* Socks5ServerDataFlow::NextHop() const {
  return data_flow_.get();
}

data_flow::DataType Socks5ServerDataFlow::FlowDataType() const {
  return DataType::Stream;
}

std::shared_ptr<utils::Session> Socks5ServerDataFlow::Session() const {
  return session_;
}

boost::asio::io_context* Socks5ServerDataFlow::io() { return data_flow_->io(); }

utils::Cancelable Socks5ServerDataFlow::Open(EventHandler handler) {
  BOOST_ASSERT(!opening_);

  opening_ = true;
  state_ = data_flow::State::Establishing;

  NEDEBUG << "Getting next hop ready.";

  open_cancelable_ = data_flow_->Open([this, handler](std::error_code ec) {
    if (ec) {
      NEERROR << "Error happened when SOCKS5 server data flow read from next "
                 "hop, error code is "
              << ec;

      state_ = data_flow::State::Closed;

      handler(ec);
      return;
    }

    NEDEBUG << "Start SOCKS5 negotiation.";
    NegotiateRead(handler);
  });

  return open_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::Continue(EventHandler handler) {
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

  auto buffer = utils::Buffer(len);
  buffer.SetByte(0, 5);
  buffer.SetByte(1, 0);
  buffer.SetByte(2, 0);
  buffer.SetByte(3, type);

  for (size_t i = 4; i < len - 4; i++) {
    buffer.SetByte(i, 0);
  }

  reportable_ = false;

  open_cancelable_ =
      data_flow_->Write(std::move(buffer), [this, handler](std::error_code ec) {
        if (ec) {
          state_ = data_flow::State::Closed;
          handler(ec);
          return;
        }

        write_cancelable_ = data_flow_->Continue(
            [this, handler, cancelable{open_cancelable_}](std::error_code ec) {
              if (cancelable.canceled()) return;
              if (ec) {
                state_ = data_flow::State::Closed;
              } else {
                state_ = data_flow::State::Established;
              }
              handler(ec);
            });
      });

  return open_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::ReportError(std::error_code error_code,
                                                    EventHandler handler) {
  (void)error_code;

  open_cancelable_.Cancel();
  read_cancelable_.Cancel();
  write_cancelable_.Cancel();

  open_cancelable_ = utils::Cancelable();
  state_ = data_flow::State::Closing;
  read_closed_ = true;
  write_closed_ = true;
  reading_ = false;
  writing_ = false;

  if (!reportable_) {
    boost::asio::post(*io(), [this, handler, cancelable{open_cancelable_},
                              lifetime{life_time_cancelable()}]() {
      if (cancelable.canceled() || lifetime.canceled()) {
        return;
      }

      state_ = data_flow::State::Closed;
      handler(ErrorCode::NoError);
    });

    return open_cancelable_;
  }

  auto buffer = utils::Buffer(10);
  buffer.SetByte(0, 5);
  buffer.SetByte(1, 1);  // TODO: Return proper error code
  buffer.SetByte(2, 0);
  buffer.SetByte(3, 1);

  write_cancelable_ =
      data_flow_->Write(std::move(buffer), [this, handler](std::error_code ec) {
        state_ = data_flow::State::Closed;
        handler(ec);
      });

  return open_cancelable_;
}

LocalDataFlowInterface* Socks5ServerDataFlow::NextLocalHop() const {
  return data_flow_.get();
}

void Socks5ServerDataFlow::EnsurePendingAuthBuffer() {
  if (!pending_auth_) {
    pending_auth_ = std::make_unique<uint8_t[]>(512);
  }
}

void Socks5ServerDataFlow::AppendToAuthBuffer(const utils::Buffer& buffer) {
  BOOST_ASSERT(pending_auth_);
  BOOST_ASSERT(pending_auth_length_ + buffer.size() <= AUTH_READ_BUFFER_SIZE);

  buffer.WalkInternalChunk(
      [this](const void* data, size_t len, void* context) {
        (void)context;
        std::memcpy(pending_auth_.get() + pending_auth_length_, data, len);
        pending_auth_length_ += len;
        return true;
      },
      0, nullptr);
}

void Socks5ServerDataFlow::NegotiateRead(EventHandler handler) {
  read_cancelable_ = data_flow_->Read(
      utils::Buffer(AUTH_READ_BUFFER_SIZE),
      [this, handler](utils::Buffer&& buffer, std::error_code ec) mutable {
        if (ec) {
          NEERROR << "Error happened when reading hello from SOCKS5 client, "
                     "error code is:"
                  << ec;

          state_ = data_flow::State::Closed;

          handler(ec);
          return;
        }

        EnsurePendingAuthBuffer();

        if (pending_auth_length_ + buffer.size() > AUTH_READ_BUFFER_SIZE) {
          NEERROR << "Hello request from client is too long";

          state_ = data_flow::State::Closed;
          handler(ErrorCode::IllegalRequest);
          return;
        }
        AppendToAuthBuffer(buffer);

        switch (negotiation_state_) {
          case NegotiateState::ReadingVersion: {
            if (pending_auth_length_ < 3) {
              NEDEBUG << "Read partial hello data with length "
                      << pending_auth_length_ << " from client. Reading more.";

              NegotiateRead(handler);
              return;
            }

            if (pending_auth_[0] != 5) {
              NEERROR << "Client send wrong socks version " << pending_auth_[0]
                      << ", only 5 is suppoted.";

              state_ = data_flow::State::Closed;
              handler(ErrorCode::UnsupportedVersion);
              return;
            }

            uint8_t len = pending_auth_[1];
            if (pending_auth_length_ < len + 2) {
              NEDEBUG << "Read partial hello data with length "
                      << pending_auth_length_ << " from client. Reading more.";

              NegotiateRead(handler);
              return;
            }

            if (pending_auth_length_ > len + 2) {
              NEERROR << "Hello request from client is too long";

              state_ = data_flow::State::Closed;
              handler(ErrorCode::IllegalRequest);
              return;
            }

            bool noauth_present = false;
            // Only support no auth now
            for (int i = 0; i < len; i++) {
              if (pending_auth_[i + 2] == 0) {
                noauth_present = true;
                break;
              }
            }

            if (!noauth_present) {
              // TODO: Send back standard response.
              state_ = data_flow::State::Closed;
              handler(ErrorCode::UnsupportedAuthenticationMethod);
              return;
            }

            buffer.ShrinkBack(buffer.size() - 2);
            buffer.SetByte(1, 0);

            pending_auth_length_ = 0;

            write_cancelable_ = data_flow_->Write(
                std::move(buffer), [this, handler](std::error_code ec) mutable {
                  if (ec) {
                    state_ = data_flow::State::Closed;
                    handler(ec);
                    return;
                  }

                  negotiation_state_ = NegotiateState::ReadingRequest;

                  NegotiateRead(handler);
                });

            return;
          }
          case NegotiateState::ReadingRequest: {
            if (pending_auth_length_ < 10) {
              NegotiateRead(handler);
              return;
            }

            if (pending_auth_[0] != 5) {
              state_ = data_flow::State::Closed;
              handler(ErrorCode::UnsupportedVersion);
              return;
            }

            if (pending_auth_[1] != 1) {
              state_ = data_flow::State::Closed;
              handler(ErrorCode::UnsupportedCommand);
              return;
            }

            if (pending_auth_[2] != 0) {
              state_ = data_flow::State::Closed;
              handler(ErrorCode::IllegalRequest);
              return;
            }

            size_t offset = 4;
            switch (pending_auth_[3]) {
              case 1: {
                if (pending_auth_length_ != 10) {
                  state_ = data_flow::State::Closed;
                  handler(ErrorCode::IllegalRequest);
                  return;
                }

                auto bytes = boost::asio::ip::address_v4::bytes_type();
                BOOST_ASSERT(bytes.size() == 4);
                std::memcpy(bytes.data(), pending_auth_.get() + offset,
                            bytes.size());
                session_->set_endpoint(std::make_shared<utils::Endpoint>(
                    boost::asio::ip::address(
                        boost::asio::ip::address_v4(bytes)),
                    0));
                offset += bytes.size();

              } break;
              case 3: {
                uint8_t len = pending_auth_[offset];
                if (pending_auth_length_ < offset + 1 + len + 2) {
                  NegotiateRead(handler);
                  return;
                }

                if (pending_auth_length_ > offset + 1 + len + 2) {
                  state_ = data_flow::State::Closed;
                  handler(ErrorCode::IllegalRequest);
                  return;
                }

                offset++;
                std::string host(
                    reinterpret_cast<char*>(pending_auth_.get()) + offset, len);

                session_->set_endpoint(
                    std::make_shared<utils::Endpoint>(host, 0));

                offset += len;
              } break;
              case 4: {
                if (pending_auth_length_ < 22) {
                  NegotiateRead(handler);
                  return;
                }

                if (pending_auth_length_ > 22) {
                  state_ = data_flow::State::Closed;
                  handler(ErrorCode::IllegalRequest);
                  return;
                }

                auto bytes = boost::asio::ip::address_v6::bytes_type();
                BOOST_ASSERT(bytes.size() == 16);
                std::memcpy(bytes.data(), pending_auth_.get() + offset,
                            bytes.size());
                session_->set_endpoint(std::make_shared<utils::Endpoint>(
                    boost::asio::ip::address(
                        boost::asio::ip::address_v6(bytes)),
                    0));
                offset += bytes.size();
              } break;
              default: {
                NEERROR << "Unsupported domain type "
                        << (int32_t)pending_auth_[3] << ".";
                state_ = data_flow::State::Closed;
                handler(ErrorCode::IllegalRequest);
                return;
              }
            }

            uint16_t port =
                *reinterpret_cast<uint16_t*>(pending_auth_.get() + offset);
            session_->endpoint()->set_port(ntohs(port));

            reportable_ = true;

            handler(ErrorCode::NoError);
          } break;
        }
      });
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
    case Socks5ServerDataFlow::ErrorCode::IllegalRequest:
      return "client send illegal request";
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
