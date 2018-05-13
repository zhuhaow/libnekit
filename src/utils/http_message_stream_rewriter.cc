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

#include "nekit/utils/http_message_stream_rewriter.h"

#include <boost/assert.hpp>

#include "nekit/config.h"

namespace nekit {
namespace utils {

class HttpMessageStreamRewriterImpl {
 public:
  enum class State {
    Init,
    Method,
    Url,
    Version,
    Status,
    HeaderField,
    HeaderValue,
    Body
  };

  HttpMessageStreamRewriterImpl(
      HttpMessageStreamRewriter* rewriter, HttpMessageStreamRewriter::Type type,
      HttpMessageStreamRewriter::EventHandler method_handler,
      HttpMessageStreamRewriter::EventHandler url_handler,
      HttpMessageStreamRewriter::EventHandler version_handler,
      HttpMessageStreamRewriter::EventHandler status_handler,
      HttpMessageStreamRewriter::EventHandler header_pair_handler) {
    rewriter_ = rewriter;
    type_ = type;

    auto t = type == HttpMessageStreamRewriter::Type::Request ? HTTP_REQUEST
                                                              : HTTP_RESPONSE;

    http_parser_init(&parser_, t);
    parser_.data = this;

    method_handler_ = method_handler;
    url_handler_ = url_handler;
    version_handler_ = version_handler;
    status_handler_ = status_handler;
    header_pair_handler_ = header_pair_handler;
  }

  ~HttpMessageStreamRewriterImpl() { free(pending_previous_buffer_); }

  bool RewriteBuffer(Buffer* buffer) {
    if (pending_buffer_length_) {
      buffer->InsertFront(pending_buffer_length_);
      buffer->SetData(0, pending_buffer_length_, pending_previous_buffer_);
      current_buffer_offset_ = pending_buffer_length_;
      pending_buffer_length_ = 0;
      free(pending_previous_buffer_);
      pending_previous_buffer_ = nullptr;
    } else {
      current_buffer_offset_ = 0;
    }

    current_buffer_ = buffer;

    while (current_buffer_offset_ != buffer->size() && !errored_) {
      // Unpause it first.
      http_parser_pause(&parser_, 0);

      buffer->WalkInternalChunk(
          [this](void* data, size_t len, void* context) {
            (void)context;

            int parsed_ = http_parser_execute(&parser_, &parser_settings_,
                                              static_cast<char*>(data), len);

            if (!(HTTP_PARSER_ERRNO(&parser_) == HPE_OK ||
                  HTTP_PARSER_ERRNO(&parser_) == HPE_PAUSED)) {
              errored_ = true;
              return false;
            }

            current_buffer_offset_ += (size_t)parsed_;

            if (set_token_) {
              if (step_back_) {
                current_token_offset_ = current_buffer_offset_ - 1;
              } else {
                current_token_offset_ = current_buffer_offset_;
              }

              set_token_ = false;
              step_back_ = false;
            }

            return false;
          },
          current_buffer_offset_, nullptr);
    }

    if (errored_) {
      return false;
    }

    if (current_token_offset_ != buffer->size()) {
      pending_buffer_length_ = buffer->size() - current_token_offset_;

      if (pending_buffer_length_ > NEKIT_HTTP_STREAM_REWRITER_MAX_BUFFER_SIZE) {
        return false;
      }

      pending_previous_buffer_ = malloc(pending_buffer_length_);
      buffer->GetData(current_token_offset_, pending_buffer_length_,
                      pending_previous_buffer_);
      buffer->ShrinkBack(pending_buffer_length_);

      current_token_end_offset_ -= current_token_offset_;
      current_header_field_offset_ -= current_token_offset_;
      current_header_field_end_offset_ -= current_token_offset_;
      current_header_value_offset_ -= current_token_offset_;
      current_header_value_end_offset_ -= current_token_offset_;
      current_token_offset_ = 0;
    }

    return true;
  }

  const HttpMessageStreamRewriter::Header& CurrentHeader() {
    if (!current_header_valid_) {
      current_header_ = {GetStringFromBuffer(current_header_field_offset_,
                                             current_header_field_end_offset_ -
                                                 current_header_field_offset_),
                         GetStringFromBuffer(current_header_value_offset_,
                                             current_header_value_end_offset_ -
                                                 current_header_value_offset_)};
    }

    return current_header_;
  }

  void RewriteCurrentHeader(const HttpMessageStreamRewriter::Header& header) {
    RewriteCurrentToken(header.first + ": " + header.second);
  }

  void DeleteCurrentHeader() {
    size_t len = current_token_end_offset_ - current_token_offset_ + 1;
    current_buffer_->Shrink(current_token_offset_, len);
  }

  const std::string& CurrentToken() {
    if (!current_token_valid_) {
      current_token_ =
          GetStringFromBuffer(current_token_offset_, current_token_end_offset_ -
                                                         current_token_offset_);
    }

    return current_token_;
  }

  void RewriteCurrentToken(const std::string& new_token) {
    size_t current_len = current_token_end_offset_ - current_token_offset_;
    size_t new_len = new_token.size();

    std::ptrdiff_t diff = new_len - current_len;
    if (diff > 0) {
      current_buffer_->Insert(current_token_offset_, diff);
    } else {
      current_buffer_->Shrink(current_token_offset_, -diff);
    }

    current_token_end_offset_ += diff;
    next_token_offset_ += diff;

    current_buffer_->SetData(current_token_offset_, new_len, new_token.c_str());
  }

 private:
#define GET_REWRITER_IMPL                        \
  HttpMessageStreamRewriterImpl* rewriter_impl = \
      static_cast<HttpMessageStreamRewriterImpl*>(parser->data)

  static int OnMessageBegin(http_parser* parser) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnMessageBegin();
  }

  int OnMessageBegin() {
    BOOST_ASSERT(state_ == State::Init);
    state_ = type_ == HttpMessageStreamRewriter::Type::Request ? State::Method
                                                               : State::Version;
    set_token_ = true;
    step_back_ = true;
    http_parser_pause(&parser_, 1);

    return 0;
  }

  static int OnHeadersComplete(http_parser* parser) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnHeadersComplete();
  }

  int OnHeadersComplete() {
    switch (state_) {
      case State::HeaderValue: {
        current_token_valid_ = false;
        current_header_valid_ = false;
        header_pair_handler_(rewriter_);

        http_parser_pause(&parser_, 1);
        state_ = State::Body;
        break;
      }
      case State::HeaderField: {
        // empty value

        current_header_value_offset_ = current_token_end_offset_;
        current_header_value_end_offset_ = current_token_end_offset_;

        current_token_valid_ = false;
        current_header_valid_ = false;

        header_pair_handler_(rewriter_);

        http_parser_pause(&parser_, 1);
        state_ = State::Body;
        break;
      }
      default:
        BOOST_ASSERT(false);
    }

    return 0;
  }

  static int OnMessageComplete(http_parser* parser) {
    GET_REWRITER_IMPL;

    BOOST_ASSERT(rewriter_impl->state_ == State::Body);

    rewriter_impl->state_ = State::Init;
    rewriter_impl->set_token_ = true;

    return 0;
  }

  static int OnChunkHeader(http_parser* parser) {
    GET_REWRITER_IMPL;

    BOOST_ASSERT(rewriter_impl->state_ == State::Body);
    rewriter_impl->set_token_ = true;

    return 0;
  }

  static int OnChunkComplete(http_parser* parser) {
    GET_REWRITER_IMPL;

    BOOST_ASSERT(rewriter_impl->state_ == State::Body);
    rewriter_impl->set_token_ = true;

    return 0;
  }

  static int OnUrl(http_parser* parser, const char* data, size_t len) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnUrl(data, len);
  }

  int OnUrl(const char* data, size_t len) {
    switch (state_) {
      case State::Url: {
        current_token_end_offset_ += len;
        break;
      }
      case State::Method: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());
        next_token_offset_ = location;

        current_token_end_offset_ = location - 1;

        current_token_valid_ = false;
        method_handler_(rewriter_);
        current_token_offset_ = next_token_offset_;
        current_token_end_offset_ = current_token_offset_ + len;
        http_parser_pause(&parser_, 1);
        state_ = State::Url;
        break;
      }
      default:
        BOOST_ASSERT(false);
    }
    return 0;
  }

  static int OnStatus(http_parser* parser, const char* data, size_t len) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnStatus(data, len);
  }

  int OnStatus(const char* data, size_t len) {
    switch (state_) {
      case State::Version: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());
        BOOST_ASSERT(location > 4);
        next_token_offset_ = location - 4;

        current_token_end_offset_ = location - 5;

        current_token_valid_ = false;
        version_handler_(rewriter_);
        current_token_offset_ = next_token_offset_;
        current_token_end_offset_ = current_token_offset_ + len + 4;
        http_parser_pause(&parser_, 1);
        state_ = State::Status;
        break;
      }
      case State::Status: {
        current_token_end_offset_ += len;
        break;
      }
      default:
        BOOST_ASSERT(false);
    }

    return 0;
  }

  static int OnHeaderField(http_parser* parser, const char* data, size_t len) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnHeaderField(data, len);
  }

  int OnHeaderField(const char* data, size_t len) {
    switch (state_) {
      case State::Url: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());
        next_token_offset_ = location;

        current_token_valid_ = false;
        url_handler_(rewriter_);

        current_token_offset_ = current_token_end_offset_ + 1;
        current_token_end_offset_ = next_token_offset_ - 2;
        current_token_valid_ = false;
        version_handler_(rewriter_);

        current_token_offset_ = next_token_offset_;
        current_token_end_offset_ = current_token_offset_ + len;
        current_header_field_offset_ = current_token_offset_;
        current_header_field_end_offset_ = current_token_end_offset_;

        http_parser_pause(&parser_, 1);
        state_ = State::HeaderField;
        break;
      }
      case State::Status: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());
        next_token_offset_ = location;

        current_token_valid_ = false;
        status_handler_(rewriter_);

        current_token_offset_ = next_token_offset_;
        current_token_end_offset_ = current_token_offset_ + len;
        current_header_field_offset_ = current_token_offset_;
        current_header_field_end_offset_ = current_token_end_offset_;

        http_parser_pause(&parser_, 1);
        state_ = State::HeaderField;
        break;
      }
      case State::HeaderField: {
        current_token_end_offset_ += len;
        current_header_field_end_offset_ += len;
        break;
      }
      case State::HeaderValue: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());
        next_token_offset_ = location;

        current_header_value_end_offset_ = current_token_end_offset_;

        current_token_valid_ = false;
        current_header_valid_ = false;
        header_pair_handler_(rewriter_);

        current_token_offset_ = next_token_offset_;
        current_token_end_offset_ = current_token_offset_ + len;
        current_header_field_offset_ = current_token_offset_;
        current_header_field_end_offset_ = current_token_end_offset_;

        http_parser_pause(&parser_, 1);
        state_ = State::HeaderField;
        break;
      }
      default:
        BOOST_ASSERT(false);
    }

    return 0;
  }
  static int OnHeaderValue(http_parser* parser, const char* data, size_t len) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnHeaderValue(data, len);
  }

  int OnHeaderValue(const char* data, size_t len) {
    switch (state_) {
      case State::HeaderValue: {
        current_token_end_offset_ += len;
        current_header_value_end_offset_ += len;
        break;
      }
      case State::HeaderField: {
        size_t location = current_buffer_->FindLocation(data);
        BOOST_ASSERT(location < current_buffer_->size());

        current_header_value_offset_ = location;
        current_header_value_end_offset_ = location + len;

        current_token_end_offset_ = current_header_value_end_offset_;

        state_ = State::HeaderValue;
        break;
      }
      default:
        BOOST_ASSERT(false);
    }

    return 0;
  }

  static int OnBody(http_parser* parser, const char* data, size_t len) {
    GET_REWRITER_IMPL;

    return rewriter_impl->OnBody(data, len);
  }

  int OnBody(const char* data, size_t len) {
    (void)data;
    (void)len;
    set_token_ = true;
    return 0;
  }

  std::string GetStringFromBuffer(size_t offset, size_t len) {
    char* str = static_cast<char*>(malloc(len));
    current_buffer_->GetData(offset, len, str);
    std::string output = std::string(str, len);
    free(str);
    return output;
  }

  static const http_parser_settings parser_settings_;

  HttpMessageStreamRewriter* rewriter_;

  http_parser parser_;
  HttpMessageStreamRewriter::EventHandler method_handler_, version_handler_,
      url_handler_, status_handler_, header_pair_handler_, body_handler_;

  std::pair<std::string, std::string> current_header_;
  std::string current_token_;
  bool current_token_valid_{false};
  bool current_header_valid_{false};

  Buffer* current_buffer_;

  void* pending_previous_buffer_{nullptr};
  size_t pending_buffer_length_{0};

  size_t current_buffer_offset_{0};
  size_t current_token_offset_{0};
  size_t current_token_end_offset_{0};
  size_t current_header_field_offset_{0}, current_header_field_end_offset_{0},
      current_header_value_offset_{0}, current_header_value_end_offset_{0};
  size_t next_token_offset_{0};

  bool errored_{false};

  State state_{State::Init};
  HttpMessageStreamRewriter::Type type_;
  bool set_token_{false}, step_back_{false};
};

const http_parser_settings HttpMessageStreamRewriterImpl::parser_settings_ = {
    &HttpMessageStreamRewriterImpl::OnMessageBegin,
    &HttpMessageStreamRewriterImpl::OnUrl,
    &HttpMessageStreamRewriterImpl::OnStatus,
    &HttpMessageStreamRewriterImpl::OnHeaderField,
    &HttpMessageStreamRewriterImpl::OnHeaderValue,
    &HttpMessageStreamRewriterImpl::OnHeadersComplete,
    &HttpMessageStreamRewriterImpl::OnBody,
    &HttpMessageStreamRewriterImpl::OnMessageComplete,
    &HttpMessageStreamRewriterImpl::OnChunkHeader,
    &HttpMessageStreamRewriterImpl::OnChunkComplete};

const HttpMessageStreamRewriter::EventHandler
    HttpMessageStreamRewriter::null_handler =
        [](HttpMessageStreamRewriter* rewriter) {
          (void)rewriter;
          return true;
        };

HttpMessageStreamRewriter::HttpMessageStreamRewriter(
    Type type, EventHandler method_handler, EventHandler url_handler,
    EventHandler version_handler, EventHandler status_handler,
    EventHandler header_pair_handler) {
  pimp_ = new HttpMessageStreamRewriterImpl(
      this, type, method_handler, url_handler, version_handler, status_handler,
      header_pair_handler);
}

HttpMessageStreamRewriter::~HttpMessageStreamRewriter() { delete pimp_; }

bool HttpMessageStreamRewriter::RewriteBuffer(Buffer* buffer) {
  return pimp_->RewriteBuffer(buffer);
}

const HttpMessageStreamRewriter::Header&
HttpMessageStreamRewriter::CurrentHeader() {
  return pimp_->CurrentHeader();
}

void HttpMessageStreamRewriter::RewriteCurrentHeader(const Header& header) {
  pimp_->RewriteCurrentHeader(header);
}

void HttpMessageStreamRewriter::DeleteCurrentHeader() {
  pimp_->DeleteCurrentHeader();
}

const std::string& HttpMessageStreamRewriter::CurrentToken() {
  return pimp_->CurrentToken();
}

void HttpMessageStreamRewriter::RewriteCurrentToken(
    const std::string& new_token) {
  pimp_->RewriteCurrentToken(new_token);
}

}  // namespace utils
}  // namespace nekit
