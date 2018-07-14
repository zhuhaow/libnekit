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

#include "nekit/data_flow/http_server_data_flow.h"

#include <boost/algorithm/string.hpp>

#include "nekit/transport/error_code.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#define AUTH_READ_BUFFER_SIZE 512

namespace nekit {
namespace data_flow {

class HttpServerHeaderRewriterDelegate
    : public utils::HttpMessageStreamRewriterDelegateInterface {
 public:
  HttpServerHeaderRewriterDelegate(HttpServerDataFlow* flow) : flow_{flow} {}

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
  HttpServerDataFlow* flow_;
};

HttpServerDataFlow::HttpServerDataFlow(
    std::unique_ptr<LocalDataFlowInterface>&& data_flow,
    std::shared_ptr<utils::Session> session)
    : data_flow_{std::move(data_flow)},
      session_{session},
      rewriter_{utils::HttpMessageStreamRewriter::Type::Request,
                std::make_shared<HttpServerHeaderRewriterDelegate>(this)} {
  BOOST_ASSERT_MSG(data_flow_->FlowDataType() == DataType::Stream,
                   "Packet type is not supported yet.");
}

HttpServerDataFlow::~HttpServerDataFlow() {
  open_cancelable_.Cancel();
  read_cancelable_.Cancel();
  write_cancelable_.Cancel();
}

utils::Cancelable HttpServerDataFlow::Read(utils::Buffer&& buffer,
                                           DataEventHandler handler) {
  state_machine_.ReadBegin();

  if (first_header_) {
    read_cancelable_ = utils::Cancelable();
    boost::asio::post(*io(), [this, handler, cancelable{read_cancelable_}]() {
      if (cancelable.canceled()) {
        return;
      }

      state_machine_.ReadEnd();
      handler(std::move(first_header_), ErrorCode::NoError);
    });

    return read_cancelable_;
  }

  read_cancelable_ = data_flow_->Read(
      std::move(buffer),
      [this, handler](utils::Buffer&& buffer, std::error_code ec) {
        state_machine_.ReadEnd();

        if (ec) {
          if (ec == transport::ErrorCode::EndOfFile) {
            state_machine_.ReadClosed();
            NEDEBUG << "Data flow got EOF.";
          } else {
            NEERROR << "Reading from data flow failed due to " << ec << ".";
            state_machine_.Errored();
            // report and connect cancelable should not be in use.
            write_cancelable_.Cancel();
          }

          handler(std::move(buffer), ec);
          return;
        }

        if (!is_connect_) {
          if (!rewriter_.RewriteBuffer(&buffer)) {
            state_machine_.Errored();
            handler(std::move(buffer), ErrorCode::InvalidRequest);
            return;
          }
        }

        handler(std::move(buffer), ec);
      });

  return read_cancelable_;
}

utils::Cancelable HttpServerDataFlow::Write(utils::Buffer&& buffer,
                                            EventHandler handler) {
  state_machine_.WriteBegin();

  write_cancelable_ =
      data_flow_->Write(std::move(buffer), [this, handler](std::error_code ec) {
        state_machine_.WriteEnd();

        if (ec) {
          NEERROR << "Write to data flow failed due to " << ec << ".";

          state_machine_.Errored();
        }
        handler(ec);
      });

  return write_cancelable_;
}

utils::Cancelable HttpServerDataFlow::CloseWrite(EventHandler handler) {
  state_machine_.WriteCloseBegin();

  write_cancelable_ =
      data_flow_->CloseWrite([this, handler](std::error_code ec) {
        state_machine_.WriteCloseEnd();

        if (ec) {
          state_machine_.Errored();
        }

        handler(ec);
      });

  return write_cancelable_;
}

const data_flow::FlowStateMachine& HttpServerDataFlow::StateMachine() const {
  return state_machine_;
}

data_flow::DataFlowInterface* HttpServerDataFlow::NextHop() const {
  return data_flow_.get();
}

data_flow::DataType HttpServerDataFlow::FlowDataType() const {
  return DataType::Stream;
}

std::shared_ptr<utils::Session> HttpServerDataFlow::Session() const {
  return session_;
}

boost::asio::io_context* HttpServerDataFlow::io() { return data_flow_->io(); }

utils::Cancelable HttpServerDataFlow::Open(EventHandler handler) {
  state_machine_.ConnectBegin();

  NEDEBUG << "Getting next hop ready.";

  open_cancelable_ = data_flow_->Open([this, handler](std::error_code ec) {
    if (ec) {
      NEERROR << "Error happened when HTTP server data flow read from next "
                 "hop, error code is "
              << ec;

      state_machine_.Errored();

      handler(ec);
      return;
    }

    NEDEBUG << "Start HTTP proxy negotiation.";
    NegotiateRead(handler);
  });

  return open_cancelable_;
}

utils::Cancelable HttpServerDataFlow::Continue(EventHandler handler) {
  if (is_connect_) {
    static std::string connect_response_header =
        "HTTP/1.1 200 Connection Established\r\n\r\n";

    auto buffer = utils::Buffer(connect_response_header.size());
    buffer.SetData(0, connect_response_header.size(),
                   connect_response_header.c_str());

    open_cancelable_ = data_flow_->Write(
        std::move(buffer), [this, handler](std::error_code ec) {
          if (ec) {
            state_machine_.Errored();
            handler(ec);
            return;
          }

          write_cancelable_ = data_flow_->Continue(
              [this, handler,
               cancelable{open_cancelable_}](std::error_code ec) {
                if (cancelable.canceled()) return;
                if (ec) {
                  state_machine_.Errored();
                } else {
                  state_machine_.Connected();
                }
                handler(ec);
              });
        });
  } else {
    open_cancelable_ = data_flow_->Continue(
        [this, handler, cancelable{open_cancelable_}](std::error_code ec) {
          if (cancelable.canceled()) return;
          if (ec) {
            state_machine_.Errored();
          } else {
            state_machine_.Connected();
          }
          handler(ec);
        });
  }

  return open_cancelable_;
}

void HttpServerDataFlow::NegotiateRead(EventHandler handler) {
  read_cancelable_ = data_flow_->Read(
      utils::Buffer(AUTH_READ_BUFFER_SIZE),
      [this, handler](utils::Buffer&& buffer, std::error_code ec) mutable {
        if (ec) {
          NEERROR
              << "Error happened when reading first request from HTTP client, "
                 "error code is:"
              << ec;

          state_machine_.Errored();

          handler(ec);
          return;
        }

        if (!rewriter_.RewriteBuffer(&buffer)) {
          NEERROR << "Error happened when reading first request from HTTP "
                     "client, request is invalid.";

          state_machine_.Errored();

          handler(ErrorCode::InvalidRequest);
          return;
        }

        if (has_read_method_) {
          if (is_connect_) {
            if (reading_first_header_) {
              NegotiateRead(handler);
              return;
            } else {
              if (buffer.size() != first_header_offset_) {
                state_machine_.Errored();
                handler(ErrorCode::DataBeforeConnectRequestFinish);
                return;
              }

              handler(ErrorCode::NoError);
              return;
            }
          }
        }

        first_header_.InsertBack(std::move(buffer));
        if (reading_first_header_) {
          NegotiateRead(handler);
          return;
        } else {
          handler(ErrorCode::NoError);
          return;
        }
      });
}

bool HttpServerDataFlow::OnMethod() {
  if (!has_read_method_) {
    has_read_method_ = true;
    if (rewriter_.CurrentToken() == "CONNECT") {
      is_connect_ = true;
    }
  }

  return true;
}

bool HttpServerDataFlow::OnUrl() {
  if (is_connect_) {
    BOOST_ASSERT(reading_first_header_);
    const auto& result = rewriter_.CurrentToken();
    http_parser_url_init(&url_parser_);
    if (http_parser_parse_url(result.c_str(), result.size(), is_connect_,
                              &url_parser_)) {
      return false;
    }

    if (!(url_parser_.field_set >> UF_HOST &&
          url_parser_.field_set >> UF_PORT)) {
      return false;
    }

    std::string host{result.c_str() + url_parser_.field_data[UF_HOST].off,
                     url_parser_.field_data[UF_HOST].len};
    session_->set_endpoint(
        std::make_shared<utils::Endpoint>(host, url_parser_.port));
    return true;
  } else {
    const auto& result = rewriter_.CurrentToken();
    http_parser_url_init(&url_parser_);
    if (http_parser_parse_url(result.c_str(), result.size(), is_connect_,
                              &url_parser_)) {
      return false;
    }

    std::string new_url;
    if (url_parser_.field_set >> UF_PATH) {
      new_url =
          std::string{result.c_str() + url_parser_.field_data[UF_PATH].off};
    } else {
      new_url = "/";
    }

    if (reading_first_header_) {
      if (!(url_parser_.field_set >> UF_HOST)) {
        return false;
      }

      std::string host{result.c_str() + url_parser_.field_data[UF_HOST].off,
                       url_parser_.field_data[UF_HOST].len};

      uint16_t port = 80;
      if (url_parser_.field_set >> UF_PORT) {
        if (url_parser_.port) {
          port = url_parser_.port;
        }
      } else {
        if (url_parser_.field_set >> UF_SCHEMA) {
          std::string scheme = std::string{
              result.c_str() + url_parser_.field_data[UF_SCHEMA].off,
              url_parser_.field_data[UF_SCHEMA].len};
          if (boost::iequals("https", scheme)) {
            port = 443;
          }
        }
      }

      session_->set_endpoint(std::make_shared<utils::Endpoint>(host, port));
    }

    rewriter_.RewriteCurrentToken(new_url);

    return true;
  }
}

bool HttpServerDataFlow::OnVersion() { return true; }

bool HttpServerDataFlow::OnStatus() { return true; }

bool HttpServerDataFlow::OnHeaderPair() {
  if (is_connect_) {
    return true;
  }

  if (boost::iequals(rewriter_.CurrentHeader().first, "Proxy-Authorization")) {
    rewriter_.DeleteCurrentHeader();
  }

  if (boost::iequals(rewriter_.CurrentHeader().first, "Proxy-Connection")) {
    rewriter_.RewriteCurrentHeader(
        {"Connection", rewriter_.CurrentHeader().second});
  }
  return true;
}

bool HttpServerDataFlow::OnHeaderComplete() { return true; }

bool HttpServerDataFlow::OnMessageComplete(size_t buffer_offset, bool upgrade) {
  (void)upgrade;
  first_header_offset_ = buffer_offset;
  reading_first_header_ = false;
  return true;
}

namespace {
struct HttpServerDataFlowErrorCategory : std::error_category {
  const char* name() const noexcept override;
  std::string message(int) const override;
};

const char* HttpServerDataFlowErrorCategory::name() const BOOST_NOEXCEPT {
  return "HTTP server stream coder";
}

std::string HttpServerDataFlowErrorCategory::message(int error_code) const {
  switch (static_cast<HttpServerDataFlow::ErrorCode>(error_code)) {
    case HttpServerDataFlow::ErrorCode::NoError:
      return "no error";
    case HttpServerDataFlow::ErrorCode::DataBeforeConnectRequestFinish:
      return "client send more data before CONNECT request finished";
    case HttpServerDataFlow::ErrorCode::InvalidRequest:
      return "client send an invalid request";
  }
}

const HttpServerDataFlowErrorCategory httpServerDataFlowErrorCategory{};

}  // namespace

std::error_code make_error_code(HttpServerDataFlow::ErrorCode ec) {
  return {static_cast<int>(ec), httpServerDataFlowErrorCategory};
}
}  // namespace data_flow
}  // namespace nekit
