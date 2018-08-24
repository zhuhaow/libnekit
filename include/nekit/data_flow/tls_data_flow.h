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

#include "../crypto/tls_tunnel.h"
#include "remote_data_flow_interface.h"

namespace nekit {
namespace data_flow {
class TlsDataFlow final : public data_flow::RemoteDataFlowInterface {
 public:
  explicit TlsDataFlow(
      std::shared_ptr<utils::Session> session, std::shared_ptr<SSL_CTX> ctx,
      std::unique_ptr<data_flow::RemoteDataFlowInterface>&& data_flow);
  ~TlsDataFlow();

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Read(utils::Buffer&&,
                                                   DataEventHandler) override;
  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Write(utils::Buffer&&,
                                                    EventHandler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable CloseWrite(EventHandler) override;

  const data_flow::FlowStateMachine& StateMachine() const override;

  data_flow::DataFlowInterface* NextHop() const override;

  std::shared_ptr<utils::Endpoint> ConnectingTo() override;

  data_flow::DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  utils::Runloop* GetRunloop() override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Connect(
      std::shared_ptr<utils::Endpoint>, EventHandler) override;

  RemoteDataFlowInterface* NextRemoteHop() const override;

 private:
  void Process();
  void HandShake();
  void TryRead();
  void TryWrite();
  void TryReadNextHop();
  void TryWriteNextHop();

  bool ReportError(utils::Error error, bool try_read_first);
  bool ReadReportError(utils::Error error);
  bool WriteReportError(utils::Error error);

  std::shared_ptr<utils::Session> session_;
  std::shared_ptr<utils::Endpoint> connect_to_;

  utils::Cancelable read_cancelable_, write_cancelable_, connect_cancelable_,
      next_read_cancelable_, next_write_cancelable_;
  data_flow::FlowStateMachine state_machine_{data_flow::FlowType::Remote};
  utils::Error pending_error_;
  bool error_reported_{false};

  DataEventHandler read_handler_;
  EventHandler connect_handler_, write_handler_;

  crypto::TlsTunnel tunnel_;

  std::unique_ptr<data_flow::RemoteDataFlowInterface> data_flow_;
};
}  // namespace data_flow
}  // namespace nekit
