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

#include <string>
#include <utility>

#include "../third_party/http_parser/http_parser.h"
#include "http_message_rewriter_interface.h"
#include "result.h"

#define NE_HTTP_MESSAGE_STREAM_PARSE_ERROR_INFO_KEY 1

namespace nekit {
namespace utils {

enum class HttpMessageStreamRewriterErrorCode {
  ParserError = 1,
  BlockTooLong,
  UserError
};

class HttpMessageStreamRewriterErrorCategory : public ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(HttpMessageStreamRewriterErrorCategory)

  static Error FromParserError(decltype(http_parser::http_errno) error);

  std::string Description(const Error& error) const override;
  std::string DebugDescription(const Error& error) const override;
};

class HttpMessageStreamRewriter;

class HttpMessageStreamRewriterImpl;

class HttpMessageStreamRewriterDelegateInterface {
 public:
  virtual ~HttpMessageStreamRewriterDelegateInterface() = default;

  virtual bool OnMethod(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnUrl(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnVersion(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnStatus(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnHeaderComplete(HttpMessageStreamRewriter* rewriter) {
    (void)rewriter;
    return true;
  }

  virtual bool OnMessageComplete(HttpMessageStreamRewriter* rewriter,
                                 size_t buffer_offset, bool upgrade) {
    (void)rewriter;
    (void)buffer_offset;
    (void)upgrade;

    return true;
  }
};

class HttpMessageStreamRewriter : public HttpMessageRewriterInterface {
 public:
  using Header = std::pair<std::string, std::string>;

  enum class Type { Request, Response };

  HttpMessageStreamRewriter(
      Type type,
      std::shared_ptr<HttpMessageStreamRewriterDelegateInterface> delegate);
  ~HttpMessageStreamRewriter();

  utils::Result<void> RewriteBuffer(Buffer* buffer) override;

  const Header& CurrentHeader();
  void RewriteCurrentHeader(const Header& header);
  void DeleteCurrentHeader();
  void AddHeader(const HttpMessageStreamRewriter::Header& header);
  void AddHeader(const std::string& header);

  const std::string& CurrentToken();
  void RewriteCurrentToken(const std::string& new_token);
  void SetSkipBodyInResponse(bool skip);

 private:
  HttpMessageStreamRewriterImpl* pimp_;
};

NE_DEFINE_NEW_ERROR_CODE(HttpMessageStreamRewriter)
}  // namespace utils
}  // namespace nekit
