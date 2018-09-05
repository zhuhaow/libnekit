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

#include "nekit/config.h"
#include "nekit/utils/common_error.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

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

utils::Cancelable Socks5ServerDataFlow::Read(DataEventHandler handler) {
  state_machine_.ReadBegin();

  read_cancelable_ =
      data_flow_->Read([this, handler](utils::Result<utils::Buffer>&& buffer) {
        state_machine_.ReadEnd();

        if (!buffer) {
          if (utils::CommonErrorCategory::IsEof(buffer.error())) {
            state_machine_.ReadClosed();
            NEDEBUG << "Data flow got EOF.";
          } else {
            NEERROR << "Reading from data flow failed due to " << buffer.error()
                    << ".";
            state_machine_.Errored();
            // report and connect cancelable should not be in use.
            write_cancelable_.Cancel();
          }
        }

        handler(std::move(buffer));
      });

  return read_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::Write(utils::Buffer&& buffer,
                                              EventHandler handler) {
  state_machine_.WriteBegin();

  write_cancelable_ = data_flow_->Write(
      std::move(buffer), [this, handler](utils::Result<void>&& result) {
        state_machine_.WriteEnd();

        if (!result) {
          NEERROR << "Write to data flow failed due to " << result.error()
                  << ".";

          state_machine_.Errored();
          // report and connect cancelable should not be in use.
          read_cancelable_.Cancel();
        }

        handler(std::move(result));
      });

  return write_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::CloseWrite(EventHandler handler) {
  state_machine_.WriteCloseBegin();

  write_cancelable_ =
      data_flow_->CloseWrite([this, handler](utils::Result<void>&& result) {
        state_machine_.WriteCloseEnd();

        if (!result) {
          state_machine_.Errored();
        }

        handler(std::move(result));
      });

  return write_cancelable_;
}

const FlowStateMachine& Socks5ServerDataFlow::StateMachine() const {
  return state_machine_;
}

DataFlowInterface* Socks5ServerDataFlow::NextHop() const {
  return data_flow_.get();
}

DataType Socks5ServerDataFlow::FlowDataType() const { return DataType::Stream; }

std::shared_ptr<utils::Session> Socks5ServerDataFlow::Session() const {
  return session_;
}

utils::Runloop* Socks5ServerDataFlow::GetRunloop() {
  return data_flow_->GetRunloop();
}

utils::Cancelable Socks5ServerDataFlow::Open(EventHandler handler) {
  state_machine_.ConnectBegin();

  NEDEBUG << "Getting next hop ready.";

  open_cancelable_ = data_flow_->Open([this,
                                       handler](utils::Result<void>&& result) {
    if (!result) {
      NEERROR << "Error happened when SOCKS5 server data flow read from next "
                 "hop, error code is "
              << result.error();

      state_machine_.Errored();

      handler(std::move(result));
      return;
    }

    NEDEBUG << "Start SOCKS5 negotiation.";
    handler_ = handler;
    NegotiateRead();
  });

  return open_cancelable_;
}

utils::Cancelable Socks5ServerDataFlow::Continue(EventHandler handler) {
  handler_ = handler;

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
  buffer[0] = 5;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = type;

  for (size_t i = 4; i < len - 4; i++) {
    buffer[i] = 0;
  }

  open_cancelable_ = data_flow_->Write(
      std::move(buffer), [this](utils::Result<void>&& result) {
        if (!result) {
          state_machine_.Errored();
          handler_(utils::MakeErrorResult(std::move(result).error()));
          return;
        }

        write_cancelable_ = data_flow_->Continue(
            [this, cancelable{open_cancelable_}](utils::Result<void>&& result) {
              if (cancelable.canceled()) return;
              if (!result) {
                state_machine_.Errored();
                handler_(utils::MakeErrorResult(std::move(result).error()));
              } else {
                state_machine_.Connected();
                handler_({});
              }
            });
      });

  return open_cancelable_;
}

void Socks5ServerDataFlow::EnsurePendingAuthBuffer() {
  if (!pending_auth_) {
    pending_auth_ =
        std::make_unique<uint8_t[]>(NEKIT_SOCKS5_SERVER_BUFFER_SIZE);
  }
}

void Socks5ServerDataFlow::AppendToAuthBuffer(const utils::Buffer& buffer) {
  BOOST_ASSERT(pending_auth_);
  BOOST_ASSERT(pending_auth_length_ + buffer.size() <=
               NEKIT_SOCKS5_SERVER_BUFFER_SIZE);

  buffer.WalkInternalChunk(
      [this](const void* data, size_t len, void* context) {
        (void)context;
        std::memcpy(pending_auth_.get() + pending_auth_length_, data, len);
        pending_auth_length_ += len;
        return true;
      },
      0, nullptr);
}

void Socks5ServerDataFlow::NegotiateRead() {
  read_cancelable_ = data_flow_->Read([this](utils::Result<utils::Buffer>&&
                                                 buffer) {
    if (!buffer) {
      NEERROR << "Error happened when reading hello from SOCKS5 client, "
                 "error is:"
              << buffer.error();

      state_machine_.Errored();

      handler_(utils::MakeErrorResult(std::move(buffer).error()));
      return;
    }

    EnsurePendingAuthBuffer();

    if (pending_auth_length_ + buffer->size() >
        NEKIT_SOCKS5_SERVER_BUFFER_SIZE) {
      NEERROR << "Hello request from client is too long";

      state_machine_.Errored();

      handler_(utils::MakeErrorResult(Socks5ServerErrorCode::IllegalRequest));
      return;
    }

    AppendToAuthBuffer(*std::move(buffer));

    switch (negotiation_state_) {
      case NegotiateState::ReadingVersion: {
        if (pending_auth_length_ < 3) {
          NEDEBUG << "Read partial hello data with length "
                  << pending_auth_length_ << " from client. Reading more.";

          NegotiateRead();
          return;
        }

        if (pending_auth_[0] != 5) {
          NEERROR << "Client send wrong socks version " << pending_auth_[0]
                  << ", only 5 is suppoted.";

          state_machine_.Errored();

          handler_(utils::MakeErrorResult(
              Socks5ServerErrorCode::UnsupportedVersion));

          return;
        }

        uint8_t len = pending_auth_[1];
        if (pending_auth_length_ < len + 2) {
          NEDEBUG << "Read partial hello data with length "
                  << pending_auth_length_ << " from client. Reading more.";

          NegotiateRead();
          return;
        }

        if (pending_auth_length_ > len + 2) {
          NEERROR << "Hello request from client is too long";

          state_machine_.Errored();

          handler_(
              utils::MakeErrorResult(Socks5ServerErrorCode::IllegalRequest));
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
          state_machine_.Errored();

          handler_(utils::MakeErrorResult(
              Socks5ServerErrorCode::UnsupportedAuthenticationMethod));
          return;
        }

        buffer->ShrinkBack(buffer->size() - 2);
        (*buffer)[1] = 0;

        pending_auth_length_ = 0;

        write_cancelable_ = data_flow_->Write(
            *std::move(buffer), [this](utils::Result<void>&& result) mutable {
              if (!result) {
                state_machine_.Errored();

                handler_(std::move(result));
                return;
              }

              negotiation_state_ = NegotiateState::ReadingRequest;

              NegotiateRead();
            });

        return;
      }
      case NegotiateState::ReadingRequest: {
        if (pending_auth_length_ < 10) {
          NegotiateRead();
          return;
        }

        if (pending_auth_[0] != 5) {
          state_machine_.Errored();

          handler_(utils::MakeErrorResult(
              Socks5ServerErrorCode::UnsupportedVersion));
          return;
        }

        if (pending_auth_[1] != 1) {
          state_machine_.Errored();

          handler_(utils::MakeErrorResult(
              Socks5ServerErrorCode::UnsupportedCommand));
          return;
        }

        if (pending_auth_[2] != 0) {
          state_machine_.Errored();

          handler_(
              utils::MakeErrorResult(Socks5ServerErrorCode::IllegalRequest));
          return;
        }

        size_t offset = 4;
        switch (pending_auth_[3]) {
          case 1: {
            if (pending_auth_length_ != 10) {
              state_machine_.Errored();

              handler_(utils::MakeErrorResult(
                  Socks5ServerErrorCode::IllegalRequest));
              return;
            }

            auto bytes = boost::asio::ip::address_v4::bytes_type();
            BOOST_ASSERT(bytes.size() == 4);
            std::memcpy(bytes.data(), pending_auth_.get() + offset,
                        bytes.size());
            session_->set_endpoint(std::make_shared<utils::Endpoint>(
                boost::asio::ip::address(boost::asio::ip::address_v4(bytes)),
                0));
            offset += bytes.size();

          } break;
          case 3: {
            uint8_t len = pending_auth_[offset];
            if (pending_auth_length_ < offset + 1 + len + 2) {
              NegotiateRead();
              return;
            }

            if (pending_auth_length_ > offset + 1 + len + 2) {
              state_machine_.Errored();

              handler_(utils::MakeErrorResult(
                  Socks5ServerErrorCode::IllegalRequest));
              return;
            }

            offset++;
            std::string host(
                reinterpret_cast<char*>(pending_auth_.get()) + offset, len);

            session_->set_endpoint(std::make_shared<utils::Endpoint>(host, 0));

            offset += len;
          } break;
          case 4: {
            if (pending_auth_length_ < 22) {
              NegotiateRead();
              return;
            }

            if (pending_auth_length_ > 22) {
              state_machine_.Errored();

              handler_(utils::MakeErrorResult(
                  Socks5ServerErrorCode::IllegalRequest));
              return;
            }

            auto bytes = boost::asio::ip::address_v6::bytes_type();
            BOOST_ASSERT(bytes.size() == 16);
            std::memcpy(bytes.data(), pending_auth_.get() + offset,
                        bytes.size());
            session_->set_endpoint(std::make_shared<utils::Endpoint>(
                boost::asio::ip::address(boost::asio::ip::address_v6(bytes)),
                0));
            offset += bytes.size();
          } break;
          default: {
            NEERROR << "Unsupported domain type " << (int32_t)pending_auth_[3]
                    << ".";
            state_machine_.Errored();

            handler_(
                utils::MakeErrorResult(Socks5ServerErrorCode::IllegalRequest));
            return;
          }
        }

        uint16_t port =
            *reinterpret_cast<uint16_t*>(pending_auth_.get() + offset);
        session_->endpoint()->set_port(ntohs(port));

        state_machine_.Connected();

        handler_({});
      } break;
    }
  });
}

std::string Socks5ServerErrorCategory::Description(
    const utils::Error& error) const {
  switch ((Socks5ServerErrorCode)error.ErrorCode()) {
    case Socks5ServerErrorCode::IllegalRequest:
      return "client send illegal request";
    case Socks5ServerErrorCode::UnsupportedAuthenticationMethod:
      return "all client requested authentication methods are not "
             "supported";
    case Socks5ServerErrorCode::UnsupportedCommand:
      return "unknown command";
    case Socks5ServerErrorCode::UnsupportedAddressType:
      return "unknown address type";
    case Socks5ServerErrorCode::UnsupportedVersion:
      return "SOCKS version is not supported";
  }
}

std::string Socks5ServerErrorCategory::DebugDescription(
    const utils::Error& error) const {
  return Description(error);
}

}  // namespace data_flow
}  // namespace nekit
