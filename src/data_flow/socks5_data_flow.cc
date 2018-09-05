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

#include "nekit/data_flow/socks5_data_flow.h"

namespace nekit {
namespace data_flow {
Socks5DataFlow::Socks5DataFlow(
    std::shared_ptr<utils::Endpoint> server_endpoint,
    std::shared_ptr<utils::Session> session,
    std::unique_ptr<RemoteDataFlowInterface>&& data_flow)
    : server_endpoint_{server_endpoint},
      session_{session},
      data_flow_{std::move(data_flow)},
      stream_reader_{data_flow_.get()} {}

Socks5DataFlow::~Socks5DataFlow() {
  connect_cancelable_.Cancel();
  connect_action_cancelable_.Cancel();
}

utils::Cancelable Socks5DataFlow::Read(DataEventHandler handler) {
  return data_flow_->Read(handler);
}

utils::Cancelable Socks5DataFlow::Write(utils::Buffer&& buffer,
                                        EventHandler handler) {
  return data_flow_->Write(std::move(buffer), handler);
}

utils::Cancelable Socks5DataFlow::CloseWrite(EventHandler handler) {
  return data_flow_->CloseWrite(handler);
}

const FlowStateMachine& Socks5DataFlow::StateMachine() const {
  if (state_machine_.State() != FlowState::Established)
    return state_machine_;
  else
    return data_flow_->StateMachine();
}

DataFlowInterface* Socks5DataFlow::NextHop() const { return data_flow_.get(); }

DataType Socks5DataFlow::FlowDataType() const { return DataType::Stream; }

std::shared_ptr<utils::Session> Socks5DataFlow::Session() const {
  return session_;
}

utils::Runloop* Socks5DataFlow::GetRunloop() {
  return data_flow_->GetRunloop();
}

utils::Cancelable Socks5DataFlow::Connect(
    std::shared_ptr<utils::Endpoint> endpoint, EventHandler handler) {
  target_endpoint_ = endpoint;

  state_machine_.ConnectBegin();

  connect_action_cancelable_ = data_flow_->Connect(
      server_endpoint_, [this, handler, cancelable{connect_cancelable_}](
                            utils::Result<void>&& result) {
        if (cancelable.canceled()) {
          return;
        }

        if (!result) {
          state_machine_.Errored();
          handler(std::move(result));
          return;
        }

        DoNegotiation(handler);
      });

  return connect_cancelable_;
}

std::shared_ptr<utils::Endpoint> Socks5DataFlow::ConnectingTo() {
  return target_endpoint_;
}

void Socks5DataFlow::DoNegotiation(EventHandler handler) {
  utils::Buffer buffer{3};
  buffer[0] = 5;
  buffer[1] = 1;
  buffer[2] = 0;

  connect_action_cancelable_ = data_flow_->Write(
      std::move(buffer), [this, cancelable{connect_cancelable_},
                          handler](utils::Result<void>&& result) {
        if (cancelable.canceled()) {
          return;
        }

        if (!result) {
          state_machine_.Errored();
          handler(std::move(result));
          return;
        }

        connect_action_cancelable_ = stream_reader_.ReadToLength(
            2,
            [this, cancelable, handler](utils::Result<utils::Buffer>&& buffer) {
              if (cancelable.canceled()) {
                return;
              }

              if (!buffer) {
                state_machine_.Errored();
                handler(utils::MakeErrorResult(std::move(buffer).error()));
                return;
              }

              if ((*buffer)[0] != 5) {
                state_machine_.Errored();
                handler(utils::MakeErrorResult(
                    Socks5ErrorCode::ServerVersionNotSupported));
                return;
              }
              if ((*buffer)[1] != 0) {
                state_machine_.Errored();
                handler(utils::MakeErrorResult(
                    Socks5ErrorCode::AuthenticationNotSupported));
                return;
              }

              (*buffer)[1] = 1;

              size_t port_offset = 4;
              if (target_endpoint_->type() == utils::Endpoint::Type::Domain) {
                size_t domain_length = target_endpoint_->host().size();
                buffer->InsertBack(1 + 1 + 1 + domain_length + 2);
                (*buffer)[3] = 3;
                (*buffer)[4] = domain_length;
                buffer->SetData(5, domain_length,
                                target_endpoint_->host().c_str());
                port_offset += 1 + domain_length;
              } else {
                if (target_endpoint_->address().is_v4()) {
                  buffer->InsertBack(1 + 1 + 4 + 2);
                  (*buffer)[3] = 1;
                  buffer->SetData(
                      4, 4,
                      target_endpoint_->address().to_v4().to_bytes().data());
                  port_offset += 4;
                } else {
                  buffer->InsertBack(1 + 1 + 16 + 2);
                  (*buffer)[3] = 4;
                  buffer->SetData(
                      4, 16,
                      target_endpoint_->address().to_v6().to_bytes().data());
                  port_offset += 16;
                }
              }

              (*buffer)[2] = 0;

              uint16_t nport = htons(target_endpoint_->port());
              buffer->SetData(port_offset, 2, &nport);

              connect_action_cancelable_ = data_flow_->Write(
                  *std::move(buffer),
                  [this, handler, cancelable](utils::Result<void>&& result) {
                    if (cancelable.canceled()) {
                      return;
                    }

                    if (!result) {
                      state_machine_.Errored();
                      handler(std::move(result));
                      return;
                    }

                    connect_action_cancelable_ = stream_reader_.ReadToLength(
                        5, [this, handler,
                            cancelable](utils::Result<utils::Buffer>&& buffer) {
                          if (cancelable.canceled()) {
                            return;
                          }

                          if (!buffer) {
                            state_machine_.Errored();
                            handler(utils::MakeErrorResult(
                                std::move(buffer).error()));
                            return;
                          }

                          if ((*buffer)[0] != 5) {
                            state_machine_.Errored();
                            handler(utils::MakeErrorResult(
                                Socks5ErrorCode::ServerVersionNotSupported));
                            return;
                          }

                          if ((*buffer)[1] != 0) {
                            state_machine_.Errored();
                            handler(utils::MakeErrorResult(
                                Socks5ErrorCode::ConnectionFailed));
                            return;
                          }

                          if ((*buffer)[2] != 0) {
                            state_machine_.Errored();
                            handler(utils::MakeErrorResult(
                                Socks5ErrorCode::InvalidResponse));
                            return;
                          }

                          size_t len;
                          switch ((*buffer)[3]) {
                            case 1:
                              len = 5;
                              break;
                            case 3:
                              len = (*buffer)[4] + 2;
                              break;
                            case 4:
                              len = 17;
                              break;
                            default:
                              state_machine_.Errored();
                              handler(utils::MakeErrorResult(
                                  Socks5ErrorCode::InvalidResponse));
                              return;
                          }

                          connect_action_cancelable_ =
                              stream_reader_.ReadToLength(
                                  len,
                                  [this, handler, cancelable](
                                      utils::Result<utils::Buffer>&& buffer) {
                                    if (cancelable.canceled()) {
                                      return;
                                    }

                                    if (!buffer) {
                                      state_machine_.Errored();
                                      handler(utils::MakeErrorResult(
                                          std::move(buffer).error()));
                                      return;
                                    }

                                    if (stream_reader_.ConsumeRemainData()) {
                                      state_machine_.Errored();
                                      handler(utils::MakeErrorResult(
                                          Socks5ErrorCode::InvalidResponse));
                                      return;
                                    }

                                    handler({});
                                    return;
                                  });
                        });
                  });
            });
      });
}
}  // namespace data_flow
}  // namespace nekit
