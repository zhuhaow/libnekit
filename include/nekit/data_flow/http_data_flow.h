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
#include "remote_data_flow_interface.h"

namespace nekit {
namespace data_flow {
class HttpHeaderRewriterDelegate;

class HttpDataFlow : public RemoteDataFlowInterface {
 public:
  class Credential {
   public:
    std::string BasicAuthenticationEncode();

   private:
    std::string username_;
    std::string password_;
  };

  enum class ErrorCode { NoError = 0, InvalidResponse, ConnectError };

  HttpDataFlow(std::shared_ptr<utils::Endpoint> server_endpoint,
               std::shared_ptr<utils::Session> session,
               std::unique_ptr<RemoteDataFlowInterface>&& data_flow,
               std::unique_ptr<Credential> credential);

  ~HttpDataFlow();

  utils::Cancelable Read(utils::Buffer&& buffer,
                         DataEventHandler handler) override
      __attribute__((warn_unused_result));

  utils::Cancelable Write(utils::Buffer&& buffer, EventHandler handler) override
      __attribute__((warn_unused_result));

  utils::Cancelable CloseWrite(EventHandler handler) override
      __attribute__((warn_unused_result));

  const FlowStateMachine& StateMachine() const override;

  DataFlowInterface* NextHop() const override;

  DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> Session() const override;

  boost::asio::io_context* io() override;

  utils::Cancelable Connect(EventHandler handler) override
      __attribute__((warn_unused_result));

  RemoteDataFlowInterface* NextRemoteHop() const override;

  std::shared_ptr<utils::Endpoint> ConnectingTo() override;

 private:
  friend class HttpHeaderRewriterDelegate;

  void ReadResponse(EventHandler handler);

  bool OnMethod();
  bool OnUrl();
  bool OnVersion();
  bool OnStatus();
  bool OnHeaderPair();
  bool OnHeaderComplete();
  bool OnMessageComplete(size_t buffer_offset, bool upgrade);

  std::shared_ptr<utils::Endpoint> server_endpoint_;
  std::shared_ptr<utils::Endpoint> target_endpoint_;

  FlowStateMachine state_machine_{FlowType::Remote};

  std::shared_ptr<utils::Session> session_;
  std::unique_ptr<RemoteDataFlowInterface> data_flow_;
  std::unique_ptr<Credential> credential_;

  utils::Cancelable connect_cancelable_, connect_action_cancelable_,
      read_cancelable_, write_cancelable_;
  utils::HttpMessageStreamRewriter rewriter_;

  bool finish_response_{false}, success_response_{false};
  size_t header_offset_{0};
  utils::Buffer pending_payload_;
};

std::error_code make_error_code(HttpDataFlow::ErrorCode ec);
}  // namespace data_flow
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::data_flow::HttpDataFlow::ErrorCode>
    : true_type {};
}  // namespace std
