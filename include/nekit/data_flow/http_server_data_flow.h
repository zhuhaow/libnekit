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

#include "../utils/http_message_stream_rewriter.h"
#include "local_data_flow_interface.h"

namespace nekit {
namespace data_flow {
enum class HttpServerErrorCode {
  DataBeforeConnectRequestFinish = 1,
  InvalidRequest,
};

class HttpServerErrorCategory : public utils::ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(HttpServerErrorCategory)

  std::string Description(const utils::Error& error) const override;
  std::string DebugDescription(const utils::Error& error) const override;
};

class HttpServerDataFlow : public LocalDataFlowInterface {
 public:
  HttpServerDataFlow(std::unique_ptr<LocalDataFlowInterface>&& data_flow,
                     std::shared_ptr<utils::Session> session);
  ~HttpServerDataFlow();

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Read(DataEventHandler) override;
  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Write(utils::Buffer&&,
                                                    EventHandler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable CloseWrite(EventHandler) override;

  const FlowStateMachine& StateMachine() const override;

  DataFlowInterface* NextHop() const override;

  DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  utils::Runloop* GetRunloop() override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Open(EventHandler) override;

  HEDLEY_WARN_UNUSED_RESULT utils::Cancelable Continue(EventHandler) override;

  bool OnMethod();

  bool OnUrl();

  bool OnVersion();

  bool OnStatus();

  bool OnHeaderPair();

  bool OnHeaderComplete();

  bool OnMessageComplete(size_t buffer_offset, bool upgrade);

 private:
  void NegotiateRead();

  std::unique_ptr<LocalDataFlowInterface> data_flow_;
  std::shared_ptr<utils::Session> session_;

  bool is_connect_{false}, has_read_method_{false}, reading_first_header_{true};
  size_t first_header_offset_{0};

  FlowStateMachine state_machine_{FlowType::Local};

  utils::Cancelable open_cancelable_, read_cancelable_, write_cancelable_;

  utils::Buffer first_header_;

  utils::HttpMessageStreamRewriter rewriter_;
  http_parser_url url_parser_;

  EventHandler handler_;
};

}  // namespace data_flow
}  // namespace nekit

NE_DEFINE_NEW_ERROR_CODE(HttpServer, nekit, data_flow)
