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

#include "../modules/http_parser/http_parser.h"
#include "http_message_rewriter_interface.h"

namespace nekit {
namespace utils {

class HttpMessageStreamRewriter;

class HttpMessageStreamRewriterImpl;

class HttpMessageStreamRewriterDelegateInterface {
 public:
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

  virtual bool OnUpgradeComplete(HttpMessageStreamRewriter* rewriter,
                                 size_t buffer_offset) {
    (void)rewriter;
    (void)buffer_offset;
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

  bool RewriteBuffer(Buffer* buffer) override;

  const Header& CurrentHeader();
  void RewriteCurrentHeader(const Header& header);
  void DeleteCurrentHeader();

  const std::string& CurrentToken();
  void RewriteCurrentToken(const std::string& new_token);

 private:
  HttpMessageStreamRewriterImpl* pimp_;
};
}  // namespace utils
}  // namespace nekit
