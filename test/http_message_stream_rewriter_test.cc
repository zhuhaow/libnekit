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

auto method_handler = [](HttpMessageStreamRewriter* rewriter) {
  EXPECT_EQ(rewriter->CurrentToken(), "GET");
  return true;
};

auto url_handler = [](HttpMessageStreamRewriter* rewriter) {
  EXPECT_EQ(rewriter->CurrentToken(), "/index.html");
  return true;
};

auto version_handler = [](HttpMessageStreamRewriter* rewriter) {
  EXPECT_EQ(rewriter->CurrentToken(), "HTTP/1.1");
  return true;
};

auto status_handler = [](HttpMessageStreamRewriter* rewriter) {
  EXPECT_EQ(rewriter->CurrentToken(), "200 OK");
  return true;
};

auto null_handler = [](HttpMessageStreamRewriter* rewriter) {
  ADD_FAILURE();
  return false;
};

auto header_handler = [header_count =
                           0](HttpMessageStreamRewriter* rewriter) mutable {
  switch (header_count % 2) {
    case 0: {
      EXPECT_EQ(rewriter->CurrentToken(), "Host: google.com");
      auto pair =
          std::make_pair<std::string, std::string>("Host", "google.com");
      EXPECT_EQ(rewriter->CurrentHeader(), pair);
      header_count++;
      break;
    }
    case 1: {
      EXPECT_EQ(rewriter->CurrentToken(), "Content-Length: 0");
      auto pair =
          std::make_pair<std::string, std::string>("Content-Length", "0");
      EXPECT_EQ(rewriter->CurrentHeader(), pair);
      header_count++;
      break;
    }
  }
  return true;
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

  HttpMessageStreamRewriter* rewriter_{nullptr};
};

class HttpRequestTest : public HttpMessageTest {
 public:
  void ResetRewriter() {
    delete rewriter_;
    rewriter_ =
        new HttpMessageStreamRewriter{HttpMessageStreamRewriter::Type::Request,
                                      method_handler,
                                      url_handler,
                                      version_handler,
                                      null_handler,
                                      header_handler};
  }

  std::string header_ =
      "GET /index.html HTTP/1.1\r\nHost: google.com\r\nContent-Length: "
      "0\r\n\r\n";
};

class HttpResponseTest : public HttpMessageTest {
 public:
  void ResetRewriter() {
    delete rewriter_;
    rewriter_ =
        new HttpMessageStreamRewriter{HttpMessageStreamRewriter::Type::Response,
                                      null_handler,
                                      url_handler,
                                      version_handler,
                                      status_handler,
                                      header_handler};
  }

  std::string header_ =
      "HTTP/1.1 200 OK\r\nHost: google.com\r\nContent-Length: "
      "0\r\n\r\n";
};

#define TEST_PARSE_MESSAGE(type)                           \
  TEST_F(type, ParseOneHeader) {                           \
    for (size_t i = 1; i < header_.size(); i++) {          \
      ResetRewriter();                                     \
      auto buffer = GetHeaderBuffer(i, header_);           \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get())); \
      EXPECT_EQ(buffer->size(), header_.size());           \
    }                                                      \
  }                                                        \
  TEST_F(type, ParseHeadersInStandaloneBuffer) {           \
    ResetRewriter();                                       \
    for (size_t i = 1; i < header_.size(); i++) {          \
      auto buffer = GetHeaderBuffer(i, header_);           \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get())); \
      EXPECT_EQ(buffer->size(), header_.size());           \
    }                                                      \
  }                                                        \
  TEST_F(type, ParseScatteredHeader) {                     \
    std::ostringstream os;                                 \
    for (int i = 0; i < 20; i++) {                         \
      os << header_;                                       \
    }                                                      \
    std::string header = os.str();                         \
    for (size_t i = 1; i < header.size(); i++) {           \
      ResetRewriter();                                     \
      auto buffer = GetHeaderBuffer(i, header);            \
      EXPECT_TRUE(rewriter_->RewriteBuffer(buffer.get())); \
      EXPECT_EQ(buffer->size(), header.size());            \
    }                                                      \
  }

TEST_PARSE_MESSAGE(HttpRequestTest)
TEST_PARSE_MESSAGE(HttpResponseTest)
