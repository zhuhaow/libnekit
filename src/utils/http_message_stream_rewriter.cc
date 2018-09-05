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

#define ENSURE_CALLBACK(x, error) \
  if (!(x)) {                     \
    errored_ = true;              \
    return (error);               \
  }

namespace nekit {
namespace utils {

std::string HttpMessageStreamRewriterErrorCategory::Description(
    const Error& error) const {
  switch ((HttpMessageStreamRewriterErrorCode)error.ErrorCode()) {
    case HttpMessageStreamRewriterErrorCode::ParserError:
      return http_errno_description(error.GetInfo<http_errno>(
          NE_HTTP_MESSAGE_STREAM_PARSE_ERROR_INFO_KEY));
    case HttpMessageStreamRewriterErrorCode::BlockTooLong:
      return "block too long";
    case HttpMessageStreamRewriterErrorCode::UserError:
      return "user error";
  }
}

std::string HttpMessageStreamRewriterErrorCategory::DebugDescription(
    const Error& error) const {
  return Description(error);
}

Error HttpMessageStreamRewriterErrorCategory::FromParserError(
    decltype(http_parser::http_errno) error) {
  Error e = HttpMessageStreamRewriterErrorCode::ParserError;
  e.CreateInfoDict();
  e.AddInfo(NE_HTTP_MESSAGE_STREAM_PARSE_ERROR_INFO_KEY, error);
  return e;
}

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
      std::shared_ptr<HttpMessageStreamRewriterDelegateInterface> delegate) {
    rewriter_ = rewriter;
    type_ = type;
    delegate_ = delegate;

    auto t = type == HttpMessageStreamRewriter::Type::Request ? HTTP_REQUEST
                                                              : HTTP_RESPONSE;

    http_parser_init(&parser_, t);
    parser_.data = this;
  }

  ~HttpMessageStreamRewriterImpl() { free(pending_previous_buffer_); }

  utils::Result<void> RewriteBuffer(Buffer* buffer) {
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

    while (current_buffer_offset_ != buffer->size() && !errored_ && !stopped_) {
      // Unpause it first.
      http_parser_pause(&parser_, 0);

      buffer->WalkInternalChunk(
          [this](void* data, size_t len, void* context) {
            (void)context;

            int parsed = http_parser_execute(&parser_, &parser_settings_,
                                             static_cast<char*>(data), len);

            current_buffer_offset_ += (size_t)parsed;

            if (process_message_complete_) {
              if (!delegate_->OnMessageComplete(
                      rewriter_, current_buffer_offset_, parser_.upgrade)) {
                errored_ = true;
                return false;
              }

              if (parser_.upgrade) {
                stopped_ = true;
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
              }
              process_message_complete_ = false;
            }

            if (!(HTTP_PARSER_ERRNO(&parser_) == HPE_OK ||
                  HTTP_PARSER_ERRNO(&parser_) == HPE_PAUSED)) {
              errored_ = true;
              return false;
            }

            if (set_token_) {
              if (step_back_) {
                current_token_offset_ = current_buffer_offset_ - 1;
              } else {
                current_token_offset_ = current_buffer_offset_;
              }

              set_token_ = false;
              step_back_ = false;
            }

            if (process_header_complete_) {
              switch (state_) {
                case State::Url: {
                  current_token_valid_ = false;
                  if (!delegate_->OnUrl(rewriter_)) {
                    errored_ = true;
                    return false;
                  }

                  current_token_offset_ = current_token_end_offset_ + 1;
                  // Actually the current buffer is at `\n` not char
                  // passing `\n`
                  current_token_end_offset_ = current_buffer_offset_ - 3;
                  current_token_valid_ = false;
                  if (!delegate_->OnVersion(rewriter_)) {
                    errored_ = true;
                    return false;
                  }

                  current_token_offset_ = current_buffer_offset_ - 1;
                  if (!delegate_->OnHeaderComplete(rewriter_)) {
                    errored_ = true;
                    return false;
                  }

                  current_token_offset_ = current_buffer_offset_;

                  state_ = State::Body;
                  break;
                }
                case State::Status: {
                  current_token_valid_ = false;

                  if (!delegate_->OnStatus(rewriter_)) {
                    errored_ = true;
                    return false;
                  }

                  current_token_offset_ = current_buffer_offset_ - 1;
                  if (!delegate_->OnHeaderComplete(rewriter_)) {
                    errored_ = true;
                    return false;
                  }

                  current_token_offset_ = current_buffer_offset_;

                  state_ = State::Body;
                  break;
                }
                default:
                  BOOST_ASSERT(false);
              }
              process_header_complete_ = false;
            }

            return false;
          },
          current_buffer_offset_, nullptr);
    }

    if (errored_) {
      if (HTTP_PARSER_ERRNO(&parser_) == HPE_OK ||
          HTTP_PARSER_ERRNO(&parser_) == HPE_PAUSED) {
        return utils::MakeErrorResult(
            HttpMessageStreamRewriterErrorCode::UserError);
      } else {
        return utils::MakeErrorResult(
            HttpMessageStreamRewriterErrorCategory::FromParserError(
                HTTP_PARSER_ERRNO(&parser_)));
      }
    }

    if (current_token_offset_ != buffer->size()) {
      pending_buffer_length_ = buffer->size() - current_token_offset_;

      if (pending_buffer_length_ > NEKIT_HTTP_STREAM_REWRITER_MAX_BUFFER_SIZE) {
        return utils::MakeErrorResult(
            HttpMessageStreamRewriterErrorCode::BlockTooLong);
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

    return {};
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
    size_t len = current_token_end_offset_ - current_token_offset_ + 2;
    current_buffer_->Shrink(current_token_offset_, len);
    current_buffer_offset_ -= len;
    next_token_offset_ -= len;
  }

  void AddHeader(const HttpMessageStreamRewriter::Header& header) {
    AddHeader(header.first + ": " + header.second);
  }

  void AddHeader(const std::string& header) {
    size_t len = header.size();
    current_buffer_->Insert(current_token_offset_, len + 2);
    current_buffer_->SetData(current_token_offset_, header.size(),
                             header.c_str());
    (*current_buffer_)[current_token_offset_ + header.size()] = '\r';
    (*current_buffer_)[current_token_offset_ + header.size() + 1] = '\n';

    current_buffer_offset_ += (len + 2);
    current_token_offset_ += (len + 2);
    next_token_offset_ += (len + 2);
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

    current_buffer_offset_ += diff;
  }

  void SetSkipBodyInResponse(bool skip) { skip_body_ = skip; }

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
        next_token_offset_ = current_token_end_offset_ + 2;
        ENSURE_CALLBACK(delegate_->OnHeaderPair(rewriter_), -1);

        current_token_offset_ = next_token_offset_;
        ENSURE_CALLBACK(delegate_->OnHeaderComplete(rewriter_), -1);

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
        next_token_offset_ = current_token_end_offset_;

        ENSURE_CALLBACK(delegate_->OnHeaderPair(rewriter_), -1);

        current_token_offset_ = next_token_offset_;
        ENSURE_CALLBACK(delegate_->OnHeaderComplete(rewriter_), -1);

        http_parser_pause(&parser_, 1);
        state_ = State::Body;
        break;
      }
      case State::Url:
      case State::Status:
        process_header_complete_ = true;
        http_parser_pause(&parser_, 1);
        break;
      default:
        BOOST_ASSERT(false);
    }

    return skip_body_ ? 1 : 0;
  }

  static int OnMessageComplete(http_parser* parser) {
    GET_REWRITER_IMPL;

    BOOST_ASSERT(rewriter_impl->state_ == State::Body);

    rewriter_impl->state_ = State::Init;
    rewriter_impl->set_token_ = true;
    rewriter_impl->process_message_complete_ = true;

    http_parser_pause(parser, 1);

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
        ENSURE_CALLBACK(delegate_->OnMethod(rewriter_), -1);
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
        ENSURE_CALLBACK(delegate_->OnVersion(rewriter_), -1);
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
        ENSURE_CALLBACK(delegate_->OnUrl(rewriter_), -1);

        current_token_offset_ = current_token_end_offset_ + 1;
        current_token_end_offset_ = next_token_offset_ - 2;
        current_token_valid_ = false;
        ENSURE_CALLBACK(delegate_->OnVersion(rewriter_), -1);

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
        ENSURE_CALLBACK(delegate_->OnStatus(rewriter_), -1);

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
        ENSURE_CALLBACK(delegate_->OnHeaderPair(rewriter_), -1);

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

        if (len) {
          current_header_value_offset_ = location;
          current_header_value_end_offset_ = location + len;

          current_token_end_offset_ = current_header_value_end_offset_;
        } else {
          current_header_value_offset_ = location - 2;
          current_header_value_end_offset_ = location - 2;
          current_token_end_offset_ = location - 2;
        }

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
  std::shared_ptr<HttpMessageStreamRewriterDelegateInterface> delegate_;

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

  bool errored_{false}, stopped_{false};
  bool process_header_complete_{false}, process_message_complete_{false};

  State state_{State::Init};
  HttpMessageStreamRewriter::Type type_;
  bool set_token_{false}, step_back_{false};
  bool skip_body_{false};
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

HttpMessageStreamRewriter::HttpMessageStreamRewriter(
    Type type,
    std::shared_ptr<HttpMessageStreamRewriterDelegateInterface> delegate) {
  pimp_ = new HttpMessageStreamRewriterImpl(this, type, delegate);
}

HttpMessageStreamRewriter::~HttpMessageStreamRewriter() { delete pimp_; }

utils::Result<void> HttpMessageStreamRewriter::RewriteBuffer(Buffer* buffer) {
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

void HttpMessageStreamRewriter::AddHeader(
    const HttpMessageStreamRewriter::Header& header) {
  pimp_->AddHeader(header);
}

void HttpMessageStreamRewriter::AddHeader(const std::string& header) {
  pimp_->AddHeader(header);
}

const std::string& HttpMessageStreamRewriter::CurrentToken() {
  return pimp_->CurrentToken();
}

void HttpMessageStreamRewriter::RewriteCurrentToken(
    const std::string& new_token) {
  pimp_->RewriteCurrentToken(new_token);
}

void HttpMessageStreamRewriter::SetSkipBodyInResponse(bool skip) {
  pimp_->SetSkipBodyInResponse(skip);
}

}  // namespace utils
}  // namespace nekit
