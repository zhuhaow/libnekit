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
      data_flow_{std::move(data_flow)} {}

Socks5DataFlow::~Socks5DataFlow() {
  connect_cancelable_.Cancel();
  connect_action_cancelable_.Cancel();
}

utils::Cancelable Socks5DataFlow::Read(utils::Buffer&& buffer,
                                       DataEventHandler handler) {
  return data_flow_->Read(std::move(buffer), handler);
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

boost::asio::io_context* Socks5DataFlow::io() { return data_flow_->io(); }

utils::Cancelable Socks5DataFlow::Connect(
    std::shared_ptr<utils::Endpoint> endpoint, EventHandler handler) {
  target_endpoint_ = endpoint;

  state_machine_.ConnectBegin();

  connect_action_cancelable_ = data_flow_->Connect(
      server_endpoint_,
      [this, handler, cancelable{connect_cancelable_}](utils::Error error) {
        if (cancelable.canceled()) {
          return;
        }

        if (error) {
          state_machine_.Errored();
          handler(error);
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
  buffer.SetByte(0, 5);
  buffer.SetByte(1, 1);
  buffer.SetByte(2, 0);

  connect_action_cancelable_ = data_flow_->Write(
      std::move(buffer),
      [this, cancelable{connect_cancelable_}, handler](utils::Error error) {
        if (cancelable.canceled()) {
          return;
        }

        if (error) {
          state_machine_.Errored();
          handler(error);
          return;
        }

        utils::Buffer buffer{2};
        connect_action_cancelable_ = data_flow_->Read(
            std::move(buffer), [this, cancelable, handler](
                                   utils::Buffer&& buffer, utils::Error error) {
              if (cancelable.canceled()) {
                return;
              }

              if (error) {
                state_machine_.Errored();
                handler(error);
                return;
              }

              if (buffer.size() != 2 || buffer.GetByte(0) != 5 ||
                  buffer.GetByte(1) != 0) {
                state_machine_.Errored();
                handler(utils::NEKitErrorCode::GeneralError);
                return;
              }

              buffer.SetByte(1, 1);

              size_t port_offset = 4;
              if (target_endpoint_->type() == utils::Endpoint::Type::Domain) {
                size_t domain_length = target_endpoint_->host().size();
                buffer.InsertBack(1 + 1 + 1 + domain_length + 2);
                buffer.SetByte(3, 3);
                buffer.SetByte(4, domain_length);
                buffer.SetData(5, domain_length,
                               target_endpoint_->host().c_str());
                port_offset += 1 + domain_length;
              } else {
                if (target_endpoint_->address().is_v4()) {
                  buffer.InsertBack(1 + 1 + 4 + 2);
                  buffer.SetByte(3, 1);
                  buffer.SetData(
                      4, 4,
                      target_endpoint_->address().to_v4().to_bytes().data());
                  port_offset += 4;
                } else {
                  buffer.InsertBack(1 + 1 + 16 + 2);
                  buffer.SetByte(3, 4);
                  buffer.SetData(
                      4, 16,
                      target_endpoint_->address().to_v6().to_bytes().data());
                  port_offset += 16;
                }
              }

              buffer.SetByte(2, 0);

              uint16_t nport = htons(target_endpoint_->port());
              buffer.SetData(port_offset, 2, &nport);

              connect_action_cancelable_ = data_flow_->Write(
                  std::move(buffer),
                  [this, handler, cancelable](utils::Error error) {
                    if (cancelable.canceled()) {
                      return;
                    }

                    if (error) {
                      state_machine_.Errored();
                      handler(error);
                      return;
                    }

                    utils::Buffer buffer{300};
                    connect_action_cancelable_ = data_flow_->Read(
                        std::move(buffer),
                        [this, handler, cancelable](utils::Buffer&& buffer,
                                                    utils::Error error) {
                          if (cancelable.canceled()) {
                            return;
                          }

                          if (error) {
                            state_machine_.Errored();
                            handler(error);
                            return;
                          }

                          if (buffer.size() < 6) {
                            state_machine_.Errored();
                            handler(utils::NEKitErrorCode::GeneralError);
                            return;
                          }

                          if (buffer.GetByte(0) != 5 ||
                              buffer.GetByte(1) != 0 ||
                              buffer.GetByte(2) != 0) {
                            state_machine_.Errored();
                            handler(utils::NEKitErrorCode::GeneralError);
                            return;
                          }

                          uint8_t type = buffer.GetByte(3);
                          if (!((type == 1 && buffer.size() == 10) ||
                                (type == 3 &&
                                 buffer.size() ==
                                     4 + 1 + 2 + buffer.GetByte(4)) ||
                                (type == 4 && buffer.size() == 20))) {
                            state_machine_.Errored();
                            handler(utils::NEKitErrorCode::GeneralError);
                            return;
                          }
                          state_machine_.Connected();
                          handler(error);
                          return;
                        });
                  });
            });
      });
}
}  // namespace data_flow
}  // namespace nekit
