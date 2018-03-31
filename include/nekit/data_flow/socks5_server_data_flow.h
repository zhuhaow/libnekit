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
class Socks5ServerDataFlow final : public LocalDataFlowInterface,
                                   private utils::LifeTime {
 public:
  enum class ErrorCode {
    NoError = 0,
    RequestIncomplete,
    UnsupportedVersion,
    UnsupportedAuthenticationMethod,
    UnsupportedCommand,
    UnsupportedAddressType
  };

  Socks5ServerDataFlow(std::unique_ptr<LocalDataFlowInterface>&& data_flow,
                       std::shared_ptr<utils::Session> session);

  const utils::Cancelable& Read(std::unique_ptr<utils::Buffer>&&,
                          DataEventHandler) override
      __attribute__((warn_unused_result));
  const utils::Cancelable& Write(std::unique_ptr<utils::Buffer>&&,
                           EventHandler) override
      __attribute__((warn_unused_result));

  // This should cancel the current write request.
  const utils::Cancelable& CloseWrite(EventHandler) override
      __attribute__((warn_unused_result));

  bool IsReadClosed() const override;
  bool IsWriteClosed() const override;
  bool IsClosed() const override;

  bool IsReading() const override;
  bool IsWriting() const override;

  bool IsIdle() const override;

  data_flow::DataFlowInterface* NextHop() const override;

  data_flow::DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> session() const override;

  boost::asio::io_context* io() override;

  const utils::Cancelable& Open(EventHandler) override
      __attribute__((warn_unused_result));

  const utils::Cancelable& Continue(EventHandler) override
      __attribute__((warn_unused_result));

  const utils::Cancelable& ReportError(std::error_code, EventHandler) override
      __attribute__((warn_unused_result));

  LocalDataFlowInterface* NextLocalHop() const override;

 private:
  std::unique_ptr<LocalDataFlowInterface> data_flow_;
  std::shared_ptr<utils::Session> session_;

  bool reporting_{false}, forwarding_{false}, reportable_{false};
  uint8_t pending_action_{0};
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
