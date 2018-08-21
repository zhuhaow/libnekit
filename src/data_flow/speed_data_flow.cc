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

#include "nekit/data_flow/speed_data_flow.h"

namespace nekit {
namespace data_flow {

SpeedDataFlow::SpeedDataFlow(
    std::shared_ptr<utils::Session> session,
    std::vector<std::pair<std::unique_ptr<RemoteDataFlowInterface>, int>>&&
        data_flows)
    : session_{session}, data_flows_{std::move(data_flows)} {}

SpeedDataFlow::~SpeedDataFlow() { connect_cancelable_.Cancel(); }

utils::Cancelable SpeedDataFlow::Read(utils::Buffer&& buffer,
                                      DataEventHandler handler) {
  return data_flow_->Read(std::move(buffer), handler);
}

utils::Cancelable SpeedDataFlow::Write(utils::Buffer&& buffer,
                                       EventHandler handler) {
  return data_flow_->Write(std::move(buffer), handler);
}

utils::Cancelable SpeedDataFlow::CloseWrite(EventHandler handler) {
  return data_flow_->CloseWrite(handler);
}

const FlowStateMachine& SpeedDataFlow::StateMachine() const {
  if (state_machine_.State() == FlowState::Establishing ||
      state_machine_.State() == FlowState::Init ||
      state_machine_.State() == FlowState::Closed) {
    return state_machine_;
  } else {
    return data_flow_->StateMachine();
  }
}

DataFlowInterface* SpeedDataFlow::NextHop() const { return data_flow_.get(); }

DataType SpeedDataFlow::FlowDataType() const {
  return data_flows_[0].first->FlowDataType();
}

std::shared_ptr<utils::Session> SpeedDataFlow::Session() const {
  return session_;
}

utils::Runloop* SpeedDataFlow::GetRunloop() {
  return data_flows_.size() ? data_flows_[0].first->GetRunloop()
                            : data_flow_->GetRunloop();
}

utils::Cancelable SpeedDataFlow::Connect(
    std::shared_ptr<utils::Endpoint> endpoint, EventHandler handler) {
  state_machine_.ConnectBegin();

  connect_cancelable_ = utils::Cancelable();
  target_endpoint_ = endpoint;
  current_active_connection_ = data_flows_.size();
  for (size_t i = 0; i < data_flows_.size(); i++) {
    connect_timers_.emplace_back(
        GetRunloop(), [this, cancelable{connect_cancelable_}, i, handler]() {
          if (cancelable.canceled()) {
            return;
          }

          auto _ = data_flows_[i].first->Connect(
              target_endpoint_->Dup(),
              [this, cancelable, i, handler](utils::Error error) {
                if (cancelable.canceled()) {
                  return;
                }

                current_active_connection_--;
                if (error) {
                  if (current_active_connection_ == 0) {
                    handler(error);
                    return;
                  }
                }

                cancelable.canceled();

                data_flow_ = std::move(data_flows_[i].first);
                connect_timers_.clear();
                data_flows_.clear();

                state_machine_.Connected();

                handler(error);
              });
        });

    connect_timers_.back().Wait(data_flows_[i].second);
  }

  return connect_cancelable_;
}

std::shared_ptr<utils::Endpoint> SpeedDataFlow::ConnectingTo() {
  return target_endpoint_;
}
}  // namespace data_flow
}  // namespace nekit
