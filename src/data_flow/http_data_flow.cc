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

#include "nekit/data_flow/http_data_flow.h"

#include <boost/algorithm/string.hpp>

#include "nekit/transport/error_code.h"
#include "nekit/utils/base64.h"

namespace nekit {
namespace data_flow {

class HttpHeaderRewriterDelegate
    : public utils::HttpMessageStreamRewriterDelegateInterface {
 public:
  HttpHeaderRewriterDelegate(HttpDataFlow* flow) : flow_{flow} {}

  bool OnMethod(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnMethod();
  }

  bool OnUrl(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnUrl();
  }

  bool OnVersion(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnVersion();
  }

  bool OnStatus(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnStatus();
  }

  bool OnHeaderPair(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnHeaderPair();
  }

  bool OnHeaderComplete(utils::HttpMessageStreamRewriter* rewriter) override {
    (void)rewriter;
    return flow_->OnHeaderComplete();
  }

  bool OnMessageComplete(utils::HttpMessageStreamRewriter* rewriter,
                         size_t buffer_offset, bool upgrade) override {
    (void)rewriter;
    return flow_->OnMessageComplete(buffer_offset, upgrade);
  }

 private:
  HttpDataFlow* flow_;
};

std::string HttpDataFlow::Credential::BasicAuthenticationEncode() {
  return nekit::utils::Base64::Encode(username_ + ":" + password_);
}

HttpDataFlow::HttpDataFlow(std::shared_ptr<utils::Endpoint> server_endpoint,
                           std::shared_ptr<utils::Session> session,
                           std::unique_ptr<RemoteDataFlowInterface>&& data_flow,
                           std::unique_ptr<Credential> credential)
    : server_endpoint_{server_endpoint},
      session_{session},
      data_flow_{std::move(data_flow)},
      credential_{std::move(credential)},
      rewriter_{utils::HttpMessageStreamRewriter::Type::Response,
                std::make_shared<HttpHeaderRewriterDelegate>(this)} {
  rewriter_.SetSkipBodyInResponse(true);
}

HttpDataFlow::~HttpDataFlow() {
  connect_cancelable_.Cancel();
  read_cancelable_.Cancel();
  connect_action_cancelable_.Cancel();
  write_cancelable_.Cancel();
}

utils::Cancelable HttpDataFlow::Read(utils::Buffer&& buffer,
                                     DataEventHandler handler) {
  BOOST_ASSERT(state_ != data_flow::State::Closed &&
               state_ != data_flow::State::Establishing);
  BOOST_ASSERT(!reading_);
  BOOST_ASSERT(!read_closed_);

  reading_ = true;

  read_cancelable_ = data_flow_->Read(
      std::move(buffer),
      [this, handler](utils::Buffer&& buffer, std::error_code ec) {
        reading_ = false;

        if (ec) {
          if (ec == nekit::transport::ErrorCode::EndOfFile) {
            read_closed_ = true;
            if (write_closed_ && !writing_) {
              state_ = data_flow::State::Closed;
            } else {
              state_ = data_flow::State::Closing;
            }
          } else {
            read_closed_ = true;
            write_closed_ = true;
            write_cancelable_.Cancel();
            state_ = data_flow::State::Closed;
          }
          handler(std::move(buffer), ec);
          return;
        }

        handler(std::move(buffer), ec);
      });

  return read_cancelable_;
}

utils::Cancelable HttpDataFlow::Write(utils::Buffer&& buffer,
                                      EventHandler handler) {
  BOOST_ASSERT(state_ != data_flow::State::Closed &&
               state_ != data_flow::State::Establishing);
  BOOST_ASSERT(!writing_);
  BOOST_ASSERT(!write_closed_);

  writing_ = true;
  write_cancelable_ =
      data_flow_->Write(std::move(buffer), [this, handler](std::error_code ec) {
        writing_ = false;

        if (ec) {
          read_closed_ = true;
          write_closed_ = true;
          state_ = data_flow::State::Closed;
          read_cancelable_.Cancel();
        }
        handler(ec);
      });
  return write_cancelable_;
}

utils::Cancelable HttpDataFlow::CloseWrite(EventHandler handler) {
  BOOST_ASSERT(state_ != data_flow::State::Closed &&
               state_ != data_flow::State::Establishing);
  BOOST_ASSERT(!writing_);
  BOOST_ASSERT(!write_closed_);

  writing_ = true;
  write_closed_ = true;

  state_ = data_flow::State::Closing;

  write_cancelable_ =
      data_flow_->CloseWrite([this, handler](std::error_code ec) {
        writing_ = false;

        if (read_closed_) {
          state_ = data_flow::State::Closed;
        }

        handler(ec);
      });

  return write_cancelable_;
}

bool HttpDataFlow::IsReadClosed() const {
  BOOST_ASSERT(NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_));
  return read_closed_;
}

bool HttpDataFlow::IsWriteClosed() const {
  BOOST_ASSERT(NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_));
  return write_closed_;
}

bool HttpDataFlow::IsWriteClosing() const {
  BOOST_ASSERT(NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(state_));
  return write_closed_ && writing_;
}

bool HttpDataFlow::IsReading() const {
  BOOST_ASSERT(NE_DATA_FLOW_CAN_CHECK_DATA_STATE(state_));
  return reading_;
}

bool HttpDataFlow::IsWriting() const {
  BOOST_ASSERT(NE_DATA_FLOW_CAN_CHECK_DATA_STATE(state_));
  return writing_;
}

data_flow::State HttpDataFlow::State() const { return state_; }

DataFlowInterface* HttpDataFlow::NextHop() const { return data_flow_.get(); }

DataType HttpDataFlow::FlowDataType() const { return DataType::Stream; }

std::shared_ptr<utils::Session> HttpDataFlow::Session() const {
  return session_;
}

boost::asio::io_context* HttpDataFlow::io() { return session_->io(); }

utils::Cancelable HttpDataFlow::Connect(EventHandler handler) {
  BOOST_ASSERT(state_ == data_flow::State::Closed);

  target_endpoint_ = session_->current_endpoint();
  session_->set_current_endpoint(server_endpoint_);

  BOOST_ASSERT(target_endpoint_);

  state_ = data_flow::State::Establishing;

  connect_cancelable_ = utils::Cancelable();
  connect_action_cancelable_ = data_flow_->Connect(
      [this, handler, cancelable{connect_cancelable_}](std::error_code ec) {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          state_ = data_flow::State::Closed;
          handler(ec);
          return;
        };

        std::ostringstream os;
        os << "CONNECT " << target_endpoint_->host() << ":"
           << target_endpoint_->port() << " HTTP/1.1\r\n";
        os << "Host: " << target_endpoint_->host() << ":"
           << target_endpoint_->port() << "\r\n";
        if (credential_) {
          os << "Proxy-Authorization: Basic "
             << credential_->BasicAuthenticationEncode() << "\r\n";
        }
        os << "\r\n";

        std::string request = os.str();

        auto buffer = utils::Buffer(request.size());
        buffer.SetData(0, request.size(), request.c_str());

        connect_action_cancelable_ = data_flow_->Write(
            std::move(buffer),
            [this, handler, cancelable{cancelable}](std::error_code ec) {
              if (cancelable.canceled()) {
                return;
              }

              if (ec) {
                state_ = data_flow::State::Closed;
                handler(ec);
                return;
              }

              ReadResponse(handler);
            });
      });

  return connect_cancelable_;
}

RemoteDataFlowInterface* HttpDataFlow::NextRemoteHop() const {
  return data_flow_.get();
}

std::shared_ptr<utils::Endpoint> HttpDataFlow::ConnectingTo() {
  return target_endpoint_;
}

void HttpDataFlow::ReadResponse(EventHandler handler) {
  auto buffer = utils::Buffer(96);
  connect_action_cancelable_ = data_flow_->Read(
      std::move(buffer), [this, handler, cancelable{connect_cancelable_}](
                             utils::Buffer&& buffer, std::error_code ec) {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          state_ = data_flow::State::Closed;
          if (ec == nekit::transport::ErrorCode::EndOfFile) {
            handler(ErrorCode::InvalidResponse);
          } else {
            handler(ec);
          }
          return;
        }

        if (!rewriter_.RewriteBuffer(&buffer)) {
          state_ = data_flow::State::Closed;
          handler(ErrorCode::InvalidResponse);
          return;
        }

        if (finish_response_) {
          if (success_response_) {
            buffer.ShrinkFront(header_offset_);
            if (!!buffer) {
              pending_payload_ = std::move(buffer);
            }

            state_ = data_flow::State::Established;
            handler(ErrorCode::NoError);
          } else {
            state_ = data_flow::State::Closed;
            handler(ErrorCode::ConnectError);
          }
          return;
        }

        ReadResponse(handler);
      });
}

bool HttpDataFlow::OnMethod() { return false; }

bool HttpDataFlow::OnUrl() { return false; }

bool HttpDataFlow::OnVersion() { return true; }

bool HttpDataFlow::OnStatus() {
  success_response_ = boost::istarts_with(rewriter_.CurrentToken(), "200");

  return true;
}

bool HttpDataFlow::OnHeaderPair() { return true; }

bool HttpDataFlow::OnHeaderComplete() { return true; }

bool HttpDataFlow::OnMessageComplete(size_t buffer_offset, bool upgrade) {
  (void)upgrade;
  header_offset_ = buffer_offset;
  finish_response_ = true;
  return true;
}
namespace {
struct HttpDataFlowErrorCategory : std::error_category {
  const char* name() const noexcept override;
  std::string message(int) const override;
};

const char* HttpDataFlowErrorCategory::name() const BOOST_NOEXCEPT {
  return "HTTP stream coder";
}

std::string HttpDataFlowErrorCategory::message(int error_code) const {
  switch (static_cast<HttpDataFlow::ErrorCode>(error_code)) {
    case HttpDataFlow::ErrorCode::NoError:
      return "no error";
    case HttpDataFlow::ErrorCode::InvalidResponse:
      return "server send an invalid response";
    case HttpDataFlow::ErrorCode::ConnectError:
      return "connect failed since the server returns a non 200 response";
  }
}

const HttpDataFlowErrorCategory httpDataFlowErrorCategory{};
}  // namespace

std::error_code make_error_code(HttpDataFlow::ErrorCode ec) {
  return {static_cast<int>(ec), httpDataFlowErrorCategory};
}
}  // namespace data_flow
}  // namespace nekit
