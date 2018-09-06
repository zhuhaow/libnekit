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

#include "../utils/error.h"
#include "../utils/stream_reader.h"
#include "remote_data_flow_interface.h"

namespace nekit {
namespace data_flow {
enum class Socks5ErrorCode {
  ServerVersionNotSupported = 1,
  AuthenticationNotSupported,
  InvalidResponse,
  ConnectionFailed
};

class Socks5ErrorCategory : public utils::ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(Socks5ErrorCategory)

  std::string Description(const utils::Error& error) const override;
  std::string DebugDescription(const utils::Error& error) const override;
};

class Socks5DataFlow : public RemoteDataFlowInterface {
 public:
  Socks5DataFlow(std::shared_ptr<utils::Endpoint> server_endpoint,
                 std::shared_ptr<utils::Session> session,
                 std::unique_ptr<RemoteDataFlowInterface>&& data_flow);

  ~Socks5DataFlow();

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Read(
      DataEventHandler handler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Write(
      utils::Buffer&& buffer, EventHandler handler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable CloseWrite(
      EventHandler handler) override;

  const FlowStateMachine& StateMachine() const override;

  DataFlowInterface* NextHop() const override;

  DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  utils::Runloop* GetRunloop() override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Connect(
      std::shared_ptr<utils::Endpoint>, EventHandler handler) override;

  std::shared_ptr<utils::Endpoint> ConnectingTo() override;

 private:
  void DoNegotiation(EventHandler handler);

  std::shared_ptr<utils::Endpoint> server_endpoint_, target_endpoint_;

  std::shared_ptr<utils::Session> session_;

  std::unique_ptr<RemoteDataFlowInterface> data_flow_;

  FlowStateMachine state_machine_{FlowType::Remote};

  utils::Cancelable connect_cancelable_, connect_action_cancelable_;

  utils::StreamReader stream_reader_;
};

NE_DEFINE_NEW_ERROR_CODE(Socks5)
}  // namespace data_flow
}  // namespace nekit
