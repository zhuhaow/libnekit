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

#pragma once

#include "../utils/cancelable.h"
#include "../utils/stream_reader.h"
#include "local_data_flow_interface.h"

namespace nekit {
namespace data_flow {
enum class Socks5ServerErrorCode {
  IllegalRequest = 1,
  UnsupportedVersion,
  UnsupportedAuthenticationMethod,
  UnsupportedCommand,
  UnsupportedAddressType
};

class Socks5ServerErrorCategory : public utils::ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(Socks5ServerErrorCategory);

  std::string Description(const utils::Error& error) const;
  std::string DebugDescription(const utils::Error& error) const;
};

class Socks5ServerDataFlow final : public LocalDataFlowInterface {
 public:
  Socks5ServerDataFlow(std::unique_ptr<LocalDataFlowInterface>&& data_flow,
                       std::shared_ptr<utils::Session> session);
  ~Socks5ServerDataFlow();

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Read(DataEventHandler) override;
  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Write(utils::Buffer&&,
                                                    EventHandler) override;

  // This should cancel the current write request.
  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable CloseWrite(EventHandler) override;

  const FlowStateMachine& StateMachine() const override;

  DataFlowInterface* NextHop() const override;

  DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  utils::Runloop* GetRunloop() override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Open(EventHandler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Continue(EventHandler) override;

 private:
  void EnsurePendingAuthBuffer();
  void AppendToAuthBuffer(const utils::Buffer& buffer);
  void NegotiateRead();

  std::unique_ptr<LocalDataFlowInterface> data_flow_;
  std::shared_ptr<utils::Session> session_;

  std::unique_ptr<uint8_t[]> pending_auth_;
  size_t pending_auth_length_{0};
  EventHandler handler_;

  FlowStateMachine state_machine_{FlowType::Local};

  enum class NegotiateState { ReadingVersion, ReadingRequest };
  NegotiateState negotiation_state_{NegotiateState::ReadingVersion};

  utils::Cancelable open_cancelable_, read_cancelable_, write_cancelable_;
};

NE_DEFINE_NEW_ERROR_CODE(Socks5Server)
}  // namespace data_flow
}  // namespace nekit
