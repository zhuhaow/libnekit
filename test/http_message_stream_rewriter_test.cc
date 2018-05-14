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

#include <gtest/gtest.h>

#include "nekit/utils/http_message_stream_rewriter.h"

using namespace nekit::utils;

class VanillaHttpMessageParserDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
  bool OnMethod(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), "GET");
    return true;
  }

  bool OnUrl(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), "/index.html");
    return true;
  };

  bool OnVersion(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), "HTTP/1.1");
    return true;
  };

  bool OnStatus(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), "200 OK");
    return true;
  };

  bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) override {
    switch (header_count_ % 2) {
      case 0: {
        EXPECT_EQ(rewriter->CurrentToken(), "Host: google.com");
        auto pair =
            std::make_pair<std::string, std::string>("Host", "google.com");
        EXPECT_EQ(rewriter->CurrentHeader(), pair);
        header_count_++;
        break;
      }
      case 1: {
        EXPECT_EQ(rewriter->CurrentToken(), "Content-Length: 0");
        auto pair =
            std::make_pair<std::string, std::string>("Content-Length", "0");
        EXPECT_EQ(rewriter->CurrentHeader(), pair);
        header_count_++;
        break;
      }
    }
    return true;
  };

  size_t header_count_{0};
};

class HttpMessageTest : public ::testing::Test {
 public:
  virtual ~HttpMessageTest() { delete rewriter_; }

  std::unique_ptr<Buffer> GetHeaderBuffer(size_t chunk_size,
                                          std::string header) {
    auto buffer = std::make_unique<Buffer>(0);

    size_t remain = header.size();
    while (remain) {
      buffer->InsertBack(std::min(remain, chunk_size));
      remain -= std::min(remain, chunk_size);
    }

    buffer->SetData(0, header.size(), header.c_str());
    return std::move(buffer);
  }

  std::vector<std::unique_ptr<Buffer>> GetHeaderBuffers(size_t chunk_size,
                                                        std::string header) {
    auto buffers = std::vector<std::unique_ptr<Buffer>>();

    size_t remain = header.size();
    while (remain) {
      auto buffer = std::make_unique<Buffer>(std::min(remain, chunk_size));
      buffer->SetData(0, std::min(remain, chunk_size),
                      header.c_str() + (header.size() - remain));
      remain -= std::min(remain, chunk_size);
      buffers.push_back(std::move(buffer));
    }

    return std::move(buffers);
  }

  HttpMessageStreamRewriter* rewriter_{nullptr};
};

class HttpRequestTest : public HttpMessageTest {
 public:
  void ResetRewriter() {
    delete rewriter_;
    rewriter_ = new HttpMessageStreamRewriter{
        HttpMessageStreamRewriter::Type::Request,
        std::make_shared<VanillaHttpMessageParserDelegate>()};
  }

  std::string header_ =
      "GET /index.html HTTP/1.1\r\nHost: google.com\r\nContent-Length: "
      "0\r\n\r\n";
};

class HttpResponseTest : public HttpMessageTest {
 public:
  void ResetRewriter() {
    delete rewriter_;
    rewriter_ = new HttpMessageStreamRewriter{
        HttpMessageStreamRewriter::Type::Response,
        std::make_shared<VanillaHttpMessageParserDelegate>()};
  }

  std::string header_ =
      "HTTP/1.1 200 OK\r\nHost: google.com\r\nContent-Length: "
      "0\r\n\r\n";
};

#define TEST_PARSE_MESSAGE(type)                                 \
  TEST_F(type, ParseOneHeader) {                                 \
    for (size_t i = 1; i < header_.size(); i++) {                \
      ResetRewriter();                                           \
      auto buffer = GetHeaderBuffer(i, header_);                 \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get()));       \
      EXPECT_EQ(buffer->size(), header_.size());                 \
    }                                                            \
  }                                                              \
  TEST_F(type, ParseHeadersInStandaloneBuffer) {                 \
    ResetRewriter();                                             \
    for (size_t i = 1; i < header_.size(); i++) {                \
      auto buffer = GetHeaderBuffer(i, header_);                 \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get()));       \
      EXPECT_EQ(buffer->size(), header_.size());                 \
    }                                                            \
  }                                                              \
  TEST_F(type, ParseScatteredHeader) {                           \
    std::ostringstream os;                                       \
    for (int i = 0; i < 20; i++) {                               \
      os << header_;                                             \
    }                                                            \
    std::string header = os.str();                               \
    for (size_t i = 1; i < header.size(); i++) {                 \
      ResetRewriter();                                           \
      auto buffer = GetHeaderBuffer(i, header);                  \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get()));       \
      EXPECT_EQ(buffer->size(), header.size());                  \
    }                                                            \
  }                                                              \
  TEST_F(type, ParseScatteredHeaderInBufferSequence) {           \
    std::ostringstream os;                                       \
    for (int i = 0; i < 20; i++) {                               \
      os << header_;                                             \
    }                                                            \
    std::string header = os.str();                               \
    for (size_t i = 1; i < header.size(); i++) {                 \
      ResetRewriter();                                           \
      auto buffers = GetHeaderBuffers(i, header);                \
      size_t len = 0;                                            \
      for (size_t j = 0; j < buffers.size(); j++) {              \
        EXPECT_TRUE(rewriter_->RewriteBuffer(buffers[j].get())); \
        len += buffers[j]->size();                               \
      }                                                          \
      EXPECT_EQ(len, header.size());                             \
    }                                                            \
  }

TEST_PARSE_MESSAGE(HttpRequestTest)
TEST_PARSE_MESSAGE(HttpResponseTest)
