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
#include "local_data_flow_interface.h"

namespace nekit {
namespace data_flow {
class Socks5ServerDataFlow final : public LocalDataFlowInterface {
 public:
  enum class ErrorCode {
    NoError = 0,
    IllegalRequest,
    UnsupportedVersion,
    UnsupportedAuthenticationMethod,
    UnsupportedCommand,
    UnsupportedAddressType
  };

  Socks5ServerDataFlow(std::unique_ptr<LocalDataFlowInterface>&& data_flow,
                       std::shared_ptr<utils::Session> session);
  ~Socks5ServerDataFlow();

  utils::Cancelable Read(utils::Buffer&&, DataEventHandler) override
      __attribute__((warn_unused_result));
  utils::Cancelable Write(utils::Buffer&&, EventHandler) override
      __attribute__((warn_unused_result));

  // This should cancel the current write request.
  utils::Cancelable CloseWrite(EventHandler) override
      __attribute__((warn_unused_result));

  const FlowStateMachine& StateMachine() const override;

  DataFlowInterface* NextHop() const override;

  DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  boost::asio::io_context* io() override;

  utils::Cancelable Open(EventHandler) override
      __attribute__((warn_unused_result));

  utils::Cancelable Continue(EventHandler) override
      __attribute__((warn_unused_result));

 private:
  void EnsurePendingAuthBuffer();
  void AppendToAuthBuffer(const utils::Buffer& buffer);
  void NegotiateRead(EventHandler handler);

  std::unique_ptr<LocalDataFlowInterface> data_flow_;
  std::shared_ptr<utils::Session> session_;

  std::unique_ptr<uint8_t[]> pending_auth_;
  size_t pending_auth_length_{0};

  FlowStateMachine state_machine_{FlowType::Local};

  enum class NegotiateState { ReadingVersion, ReadingRequest };
  NegotiateState negotiation_state_{NegotiateState::ReadingVersion};

  utils::Cancelable open_cancelable_, read_cancelable_, write_cancelable_;
};

std::error_code make_error_code(Socks5ServerDataFlow::ErrorCode ec);
}  // namespace data_flow
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::data_flow::Socks5ServerDataFlow::ErrorCode>
    : true_type {};
}  // namespace std
