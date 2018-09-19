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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "nekit/utils/http_message_stream_rewriter.h"

#define MAX_CONTINUES_MESSAGE 3

using namespace nekit::utils;
using ::testing::_;

// MARK: Helper class

class HttpRequestMessage {
 public:
  std::string method_, url_, version_;
  std::vector<HttpMessageStreamRewriter::Header> headers_;
  std::string body_;

  const std::string& Message() {
    if (valid_) {
      return message_;
    }
    std::ostringstream os;
    os << method_ << " " << url_ << " " << version_ << "\r\n";

    for (const auto header : headers_) {
      os << header.first << ": " << header.second << "\r\n";
    }

    os << "\r\n";

    os << body_;

    message_ = os.str();
    valid_ = true;
    return message_;
  }

  bool valid_{false};
  std::string message_;
};

class HttpResponseMessage {
 public:
  std::string version_, status_;
  std::vector<HttpMessageStreamRewriter::Header> headers_;
  std::string body_;

  std::string Message() {
    std::ostringstream os;
    os << version_ << " " << status_ << "\r\n";

    for (const auto header : headers_) {
      os << header.first << ": " << header.second << "\r\n";
    }

    os << "\r\n";

    os << body_;

    return os.str();
  }
};

class HttpMessageGenerator {
 public:
  static std::unique_ptr<Buffer> GetHeaderBuffer(size_t chunk_size,
                                                 const std::string& header) {
    auto buffer = std::make_unique<Buffer>(0);

    size_t remain = header.size();
    while (remain) {
      buffer->InsertBack(std::min(remain, chunk_size));
      remain -= std::min(remain, chunk_size);
    }

    buffer->SetData(0, header.size(), header.c_str());
    return buffer;
  }

  static std::vector<std::unique_ptr<Buffer>> GetHeaderBuffers(
      size_t chunk_size, const std::string& header) {
    auto buffers = std::vector<std::unique_ptr<Buffer>>();

    size_t remain = header.size();
    while (remain) {
      auto buffer = std::make_unique<Buffer>(std::min(remain, chunk_size));
      buffer->SetData(0, std::min(remain, chunk_size),
                      header.c_str() + (header.size() - remain));
      remain -= std::min(remain, chunk_size);
      buffers.push_back(std::move(buffer));
    }

    return buffers;
  }
};

class MockHttpStreamRewriterMessageDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
 public:
  MOCK_METHOD1(OnMethod, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD1(OnUrl, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD1(OnVersion, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD1(OnStatus, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD1(OnHeaderPair, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD1(OnHeaderComplete, bool(HttpMessageStreamRewriter*));
  MOCK_METHOD3(OnMessageComplete,
               bool(HttpMessageStreamRewriter*, size_t, bool));

  void DelegateToFake(
      std::unique_ptr<HttpMessageStreamRewriterDelegateInterface> delegate,
      HttpRequestMessage message, size_t count) {
    delegate_ = std::move(delegate);

    SetUpDelegate();

    EXPECT_CALL(*this, OnMethod(_)).Times(count);
    EXPECT_CALL(*this, OnUrl(_)).Times(count);
    EXPECT_CALL(*this, OnVersion(_)).Times(count);
    EXPECT_CALL(*this, OnStatus(_)).Times(0);
    EXPECT_CALL(*this, OnHeaderPair(_)).Times(message.headers_.size() * count);
    EXPECT_CALL(*this, OnHeaderComplete(_)).Times(count);
    EXPECT_CALL(*this, OnMessageComplete(_, _, _)).Times(count);
  }

  void DelegateToFake(
      std::unique_ptr<HttpMessageStreamRewriterDelegateInterface> delegate,
      HttpResponseMessage message, size_t count) {
    delegate_ = std::move(delegate);

    SetUpDelegate();

    EXPECT_CALL(*this, OnMethod(_)).Times(0);
    EXPECT_CALL(*this, OnUrl(_)).Times(0);
    EXPECT_CALL(*this, OnVersion(_)).Times(count);
    EXPECT_CALL(*this, OnStatus(_)).Times(count);
    EXPECT_CALL(*this, OnHeaderPair(_)).Times(message.headers_.size() * count);
    EXPECT_CALL(*this, OnHeaderComplete(_)).Times(count);
    EXPECT_CALL(*this, OnMessageComplete(_, _, _)).Times(count);
  }

  void SetUpDelegate() {
    ON_CALL(*this, OnMethod(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnMethod));
    ON_CALL(*this, OnUrl(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnUrl));
    ON_CALL(*this, OnVersion(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnVersion));
    ON_CALL(*this, OnStatus(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnStatus));
    ON_CALL(*this, OnHeaderPair(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnHeaderPair));
    ON_CALL(*this, OnHeaderComplete(_))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnHeaderComplete));
    ON_CALL(*this, OnMessageComplete(_, _, _))
        .WillByDefault(::testing::Invoke(
            delegate_.get(),
            &HttpMessageStreamRewriterDelegateInterface::OnMessageComplete));
  }

  std::unique_ptr<HttpMessageStreamRewriterDelegateInterface> delegate_;
};

// TEST: Parsing request message

class VanillaHttpRequestMessageParserDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
 public:
  VanillaHttpRequestMessageParserDelegate(HttpRequestMessage message,
                                          size_t count)
      : message_{message}, count_{count} {}

  bool OnMethod(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.method_);
    return true;
  }

  bool OnUrl(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.url_);
    return true;
  };

  bool OnVersion(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.version_);
    return true;
  };

  bool OnStatus(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  };

  bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) override {
    if (!message_.headers_.size()) {
      ADD_FAILURE();
      return false;
    }

    size_t index = header_count_ % message_.headers_.size();
    EXPECT_EQ(rewriter->CurrentToken(), message_.headers_[index].first + ": " +
                                            message_.headers_[index].second);
    EXPECT_EQ(rewriter->CurrentHeader(), message_.headers_[index]);

    header_count_++;
    return true;
  };

  bool OnHeaderComplete(HttpMessageStreamRewriter* rewriter) override {
    return true;
  }

  bool OnMessageComplete(HttpMessageStreamRewriter* rewriter, size_t offset,
                         bool upgrade) override {
    count_--;

    current_header_len_ += (offset - current_buffer_len_);

    EXPECT_EQ(current_header_len_, message_.Message().size());
    current_header_len_ = 0;
    current_buffer_len_ = offset;
    return true;
  }

  void ReturnedBufferLength(size_t len) {
    current_header_len_ += len;
    current_header_len_ -= current_buffer_len_;
    current_buffer_len_ = 0;
  }

  void Done() { EXPECT_EQ(count_, 0); }

  size_t header_count_{0};
  HttpRequestMessage message_;
  size_t count_{0};
  size_t current_header_len_{0};
  size_t current_buffer_len_{0};
};

std::vector<HttpRequestMessage> request_messages = {
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Host", "google.com"}, {"Content-Length", "0"}},
     ""},
    {"GET", "/index.html", "HTTP/1.1", {}, ""},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Host", ""}, {"Content-Length", "0"}},
     ""},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Content-Length", "0"}, {"Host", ""}},
     ""},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Host", "google.com"}, {"Content-Length", "18"}},
     "ljefkji ejkalejane"},
    {"GET", "/index.html", "HTTP/1.1", {}, ""},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Host", ""}, {"Content-Length", "3"}},
     "aaa"},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Content-Length", "5"}, {"Host", ""}},
     "bbbbb"},
};

TEST(HttpMessageStreamRewriterRequestTest, ParseOneHeader) {
  for (auto message : request_messages) {
    for (size_t i = 1; i < message.Message().size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpRequestMessageParserDelegate>(message, 1);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message, 1);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Request, delegate);
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());

      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterRequestTest, ParseHeadersInStandaloneBuffer) {
  for (auto message : request_messages) {
    auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
    auto real_delegate =
        std::make_unique<VanillaHttpRequestMessageParserDelegate>(
            message, message.Message().size());
    auto delegate_ptr = real_delegate.get();
    delegate->DelegateToFake(std::move(real_delegate), message,
                             message.Message().size());

    auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
        HttpMessageStreamRewriter::Type::Request, delegate);

    for (size_t i = 1; i <= message.Message().size(); i++) {
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
    }
    delegate_ptr->Done();
  }
}

TEST(HttpMessageStreamRewriterRequestTest, ParseScatteredHeader) {
  for (auto message : request_messages) {
    std::ostringstream os;

    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    for (size_t i = 1; i <= header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpRequestMessageParserDelegate>(
              message, MAX_CONTINUES_MESSAGE);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Request, delegate);
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, header);

      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterRequestTest,
     ParseScatteredHeaderInBufferSequence) {
  for (auto message : request_messages) {
    std::ostringstream os;
    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    for (size_t i = 1; i < header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpRequestMessageParserDelegate>(
              message, MAX_CONTINUES_MESSAGE);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Request, delegate);
      auto buffers = HttpMessageGenerator::GetHeaderBuffers(i, header);

      for (size_t j = 0; j < buffers.size(); j++) {
        EXPECT_TRUE(rewriter->RewriteBuffer(buffers[j].get()));
        delegate_ptr->ReturnedBufferLength(buffers[j]->size());
      }
      delegate_ptr->Done();
    }
  }
}

class VanillaHttpResponseMessageParserDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
 public:
  VanillaHttpResponseMessageParserDelegate(HttpResponseMessage message,
                                           size_t count)
      : message_{message}, count_{count} {}

  bool OnMethod(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  }

  bool OnUrl(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  };

  bool OnVersion(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.version_);
    return true;
  };

  bool OnStatus(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.status_);
    return true;
  };

  bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) override {
    if (!message_.headers_.size()) {
      ADD_FAILURE();
      return false;
    }

    size_t index = header_count_ % message_.headers_.size();
    EXPECT_EQ(rewriter->CurrentToken(), message_.headers_[index].first + ": " +
                                            message_.headers_[index].second);
    EXPECT_EQ(rewriter->CurrentHeader(), message_.headers_[index]);

    header_count_++;
    return true;
  };

  bool OnHeaderComplete(HttpMessageStreamRewriter* rewriter) override {
    return true;
  }

  bool OnMessageComplete(HttpMessageStreamRewriter* rewriter, size_t offset,
                         bool upgrade) override {
    count_--;

    current_header_len_ += (offset - current_buffer_len_);

    EXPECT_EQ(current_header_len_, message_.Message().size());
    current_header_len_ = 0;
    current_buffer_len_ = offset;
    return true;
  }

  void ReturnedBufferLength(size_t len) {
    current_header_len_ += len;
    current_header_len_ -= current_buffer_len_;
    current_buffer_len_ = 0;
  }

  void Done() { EXPECT_EQ(count_, 0); }

  size_t header_count_{0};
  HttpResponseMessage message_;
  size_t count_{0};
  size_t current_header_len_{0};
  size_t current_buffer_len_{0};
};

std::vector<HttpResponseMessage> response_messages = {
    {"HTTP/1.1", "200 OK", {{"Date", "NOW"}, {"Content-Length", "0"}}, ""},
    {"HTTP/1.1", "200 OK", {{"Date", ""}, {"Content-Length", "0"}}, ""},
    {"HTTP/1.1", "200 OK", {{"Content-Length", "0"}, {"Date", "NOW"}}, ""},
    {"HTTP/1.1", "200 Connection Established", {}, ""},
};

TEST(HttpMessageStreamRewriterResponseTest, ParseOneHeader) {
  for (auto message : response_messages) {
    for (size_t i = 1; i < message.Message().size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpResponseMessageParserDelegate>(message,
                                                                     1);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message, 1);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Response, delegate);
      rewriter->SetSkipBodyInResponse(true);
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());

      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterResponseTest, ParseHeadersInStandaloneBuffer) {
  for (auto message : response_messages) {
    auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
    auto real_delegate =
        std::make_unique<VanillaHttpResponseMessageParserDelegate>(
            message, message.Message().size());
    auto delegate_ptr = real_delegate.get();
    delegate->DelegateToFake(std::move(real_delegate), message,
                             message.Message().size());

    auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
        HttpMessageStreamRewriter::Type::Response, delegate);
    rewriter->SetSkipBodyInResponse(true);

    size_t count = message.Message().size();

    for (size_t i = 1; i <= message.Message().size(); i++) {
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
    }
    delegate_ptr->Done();
  }
}

TEST(HttpMessageStreamRewriterResponseTest, ParseScatteredHeader) {
  for (auto message : response_messages) {
    std::ostringstream os;

    size_t count = MAX_CONTINUES_MESSAGE;
    for (int i = 0; i < count; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    for (size_t i = 1; i <= header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpResponseMessageParserDelegate>(
              message, MAX_CONTINUES_MESSAGE);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Response, delegate);
      rewriter->SetSkipBodyInResponse(true);
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, header);

      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBufferLength(buffer->size());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterResponseTest,
     ParseScatteredHeaderInBufferSequence) {
  for (auto message : response_messages) {
    std::ostringstream os;
    size_t count = MAX_CONTINUES_MESSAGE;
    for (int i = 0; i < count; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    for (size_t i = 1; i < header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpResponseMessageParserDelegate>(
              message, MAX_CONTINUES_MESSAGE);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Response, delegate);
      rewriter->SetSkipBodyInResponse(true);
      auto buffers = HttpMessageGenerator::GetHeaderBuffers(i, header);

      for (size_t j = 0; j < buffers.size(); j++) {
        EXPECT_TRUE(rewriter->RewriteBuffer(buffers[j].get()));
        delegate_ptr->ReturnedBufferLength(buffers[j]->size());
      }
      delegate_ptr->Done();
    }
  }
}

// TEST: Rewriting request message

class VanillaHttpRequestMessageRewriteDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
 public:
  VanillaHttpRequestMessageRewriteDelegate(
      HttpRequestMessage message, size_t count,
      std::vector<HttpRequestMessage> to_messages)
      : message_{message}, target_{count}, to_messages_{to_messages} {}

  bool OnMethod(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.method_);
    rewriter->RewriteCurrentToken(
        to_messages_[count_ % to_messages_.size()].method_);
    os_ << to_messages_[count_ % to_messages_.size()].method_ << " ";
    return true;
  }

  bool OnUrl(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.url_);
    rewriter->RewriteCurrentToken(
        to_messages_[count_ % to_messages_.size()].url_);
    os_ << to_messages_[count_ % to_messages_.size()].url_ << " ";
    return true;
  };

  bool OnVersion(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.version_);
    rewriter->RewriteCurrentToken(
        to_messages_[count_ % to_messages_.size()].version_);
    os_ << to_messages_[count_ % to_messages_.size()].version_ << "\r\n";
    return true;
  };

  bool OnStatus(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  };

  bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) override {
    if (!message_.headers_.size()) {
      ADD_FAILURE();
      return false;
    }

    size_t index = origin_header_count_;
    EXPECT_EQ(rewriter->CurrentToken(), message_.headers_[index].first + ": " +
                                            message_.headers_[index].second);
    EXPECT_EQ(rewriter->CurrentHeader(), message_.headers_[index]);
    origin_header_count_++;

    if (boost::iequals(rewriter->CurrentHeader().first, "Content-Length")) {
      os_ << rewriter->CurrentToken() << "\r\n";
      carryover_length_ = rewriter->CurrentToken().size() + 2;
      return true;
    }

    size_t rindex = rewrite_header_count_;
    if (to_messages_[count_ % to_messages_.size()].headers_.size() > rindex) {
      rewriter->RewriteCurrentHeader(
          to_messages_[count_ % to_messages_.size()].headers_[rindex]);
      os_ << to_messages_[count_ % to_messages_.size()].headers_[rindex].first
          << ": "
          << to_messages_[count_ % to_messages_.size()].headers_[rindex].second
          << "\r\n";
    } else {
      rewriter->DeleteCurrentHeader();
    }

    rewrite_header_count_++;
    return true;
  };

  bool OnHeaderComplete(HttpMessageStreamRewriter* rewriter) override {
    for (size_t i = rewrite_header_count_;
         i < to_messages_[count_ % to_messages_.size()].headers_.size(); i++) {
      rewriter->AddHeader(
          to_messages_[count_ % to_messages_.size()].headers_[i]);

      os_ << to_messages_[count_ % to_messages_.size()].headers_[i].first
          << ": "
          << to_messages_[count_ % to_messages_.size()].headers_[i].second
          << "\r\n";
    }

    os_ << "\r\n";
    return true;
  }

  bool OnMessageComplete(HttpMessageStreamRewriter* rewriter, size_t offset,
                         bool upgrade) override {
    origin_header_count_ = 0;
    rewrite_header_count_ = 0;

    expect_results_.push_back(os_.str());
    os_.str(std::string());

    offsets_.push_back(offset);
    current_header_len_ += (offset - current_buffer_len_);

    std::string temp = to_messages_[count_ % to_messages_.size()].Message();

    EXPECT_EQ(current_header_len_,
              to_messages_[count_ % to_messages_.size()].Message().size() +
                  message_.body_.size() + carryover_length_);
    current_header_len_ = 0;
    current_buffer_len_ = offset;
    carryover_length_ = 0;
    count_++;

    return true;
  }

  void ReturnedBuffer(Buffer* buffer) {
    current_header_len_ += buffer->size();
    current_header_len_ -= current_buffer_len_;
    current_buffer_len_ = 0;

    size_t prev = 0;
    for (auto offset : offsets_) {
      if (offset - prev) {
        char* r = (char*)std::malloc(offset - prev);
        buffer->GetData(prev, offset - prev, r);
        real_os_ << std::string(r, offset - prev);
        rewrite_results_.push_back(real_os_.str());
        real_os_.str(std::string());
        prev = offset;
        free(r);
      } else {
        rewrite_results_.push_back(real_os_.str());
        real_os_.str(std::string());
      }
    }
    offsets_.clear();

    if (buffer->size() - prev) {
      char* r = (char*)std::malloc(buffer->size() - prev);
      buffer->GetData(prev, buffer->size() - prev, r);
      real_os_ << std::string(r, buffer->size() - prev);
      free(r);
    }
  }

  void Done() {
    EXPECT_EQ(count_, target_);
    ASSERT_EQ(rewrite_results_.size(), expect_results_.size());
    for (size_t i = 0; i < rewrite_results_.size(); i++) {
      EXPECT_TRUE(boost::istarts_with(rewrite_results_[i], expect_results_[i]));
    }
  }

  size_t origin_header_count_{0}, rewrite_header_count_{0};
  HttpRequestMessage message_;
  std::vector<HttpRequestMessage> to_messages_;
  std::vector<std::string> rewrite_results_, expect_results_;
  std::vector<size_t> offsets_;
  std::ostringstream os_, real_os_;
  size_t count_{0}, target_;
  size_t current_header_len_{0};
  size_t current_buffer_len_{0};
  size_t carryover_length_{0};
};

std::vector<HttpRequestMessage> to_request_messages = {
    {"GET", "/ind.html", "HTTP/1.0", {{"Host", "google.com"}}, ""},
    {"GET", "/index.html", "HTTP/1.1", {}, ""},
    {"POST", "/inde.ht", "HTTP/1.1", {{"Host", ""}, {"Content", "0"}}, ""},
    {"GET", "/index.html", "HTTP/1.1", {{"Length", "0"}, {"Host", ""}}, ""},
    {"GET",
     "/index.html?from=nowhere",
     "HTTP/1.1",
     {{"Host", "google.com"}, {"Content-Length-", "18"}},
     ""},
    {"GET", "/index.html", "HTTP/1.1", {}, ""},
    {"GET",
     "/index.html",
     "HTTP/1.1",
     {{"Host", ""}, {"ContentLength124", "3"}},
     ""},
    {"GET", "/index.html", "HTTP/1.1", {{"Content", "5"}, {"Host", ""}}, ""},
};

TEST(HttpMessageStreamRewriterRequestTest, RewriteOneHeader) {
  for (size_t ind = 0; ind < request_messages.size(); ind++) {
    for (size_t j = 0; j < to_request_messages.size(); j++) {
      auto& message = request_messages[ind];
      auto to_messages =
          std::vector<HttpRequestMessage>{to_request_messages[j]};
      for (size_t i = 1; i < message.Message().size(); i++) {
        SCOPED_TRACE("Request Message (" + std::to_string(ind) + ") " +
                     "Rewrite to (" + std::to_string(j) +
                     "): " + "with buffer chunk size: " + std::to_string(i));

        auto delegate =
            std::make_shared<MockHttpStreamRewriterMessageDelegate>();
        auto real_delegate =
            std::make_unique<VanillaHttpRequestMessageRewriteDelegate>(
                message, 1, to_messages);
        auto delegate_ptr = real_delegate.get();
        delegate->DelegateToFake(std::move(real_delegate), message, 1);

        auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
            HttpMessageStreamRewriter::Type::Request, delegate);
        auto buffer =
            HttpMessageGenerator::GetHeaderBuffer(i, message.Message());

        EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
        delegate_ptr->ReturnedBuffer(buffer.get());
        delegate_ptr->Done();
      }
    }
  }
}

TEST(HttpMessageStreamRewriterRequestTest, RewriteHeadersInStandaloneBuffer) {
  for (size_t ind = 0; ind < request_messages.size(); ind++) {
    auto& message = request_messages[ind];
    auto& to_messages = to_request_messages;
    auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
    auto real_delegate =
        std::make_unique<VanillaHttpRequestMessageRewriteDelegate>(
            message, message.Message().size(), to_messages);
    auto delegate_ptr = real_delegate.get();
    delegate->DelegateToFake(std::move(real_delegate), message,
                             message.Message().size());

    auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
        HttpMessageStreamRewriter::Type::Request, delegate);

    for (size_t i = 1; i <= message.Message().size(); i++) {
      SCOPED_TRACE("Request Message (" + std::to_string(ind) + ") " +
                   "with buffer chunk size: " + std::to_string(i));

      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBuffer(buffer.get());
    }
    delegate_ptr->Done();
  }
}

TEST(HttpMessageStreamRewriterRequestTest, RewriteScatteredHeader) {
  for (size_t ind = 0; ind < request_messages.size(); ind++) {
    auto& message = request_messages[ind];
    std::ostringstream os;

    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    auto& to_messages = to_request_messages;

    for (size_t i = 1; i < header.size(); i++) {
      SCOPED_TRACE("Request Message (" + std::to_string(ind) + ") " +
                   "with buffer chunk size: " + std::to_string(i));

      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpRequestMessageRewriteDelegate>(
              message, MAX_CONTINUES_MESSAGE, to_messages);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Request, delegate);
      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, header);
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBuffer(buffer.get());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterRequestTest,
     RewriteScatteredHeaderInBufferSequence) {
  for (size_t ind = 0; ind < request_messages.size(); ind++) {
    auto& message = request_messages[ind];
    std::ostringstream os;

    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    auto& to_messages = to_request_messages;

    for (size_t i = 1; i < header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpRequestMessageRewriteDelegate>(
              message, MAX_CONTINUES_MESSAGE, to_messages);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Request, delegate);
      auto buffers = HttpMessageGenerator::GetHeaderBuffers(i, header);

      for (size_t j = 0; j < buffers.size(); j++) {
        SCOPED_TRACE("Request Message (" + std::to_string(ind) + ") " +
                     "with buffer chunk size: " + std::to_string(i) +
                     ", iter: " + std::to_string(j));

        EXPECT_TRUE(rewriter->RewriteBuffer(buffers[j].get()));
        delegate_ptr->ReturnedBuffer(buffers[j].get());
      }
      delegate_ptr->Done();
    }
  }
}

// TEST: Rewriting response message

class VanillaHttpResponseMessageRewriteDelegate
    : public HttpMessageStreamRewriterDelegateInterface {
 public:
  VanillaHttpResponseMessageRewriteDelegate(
      HttpResponseMessage message, size_t count,
      std::vector<HttpResponseMessage> to_messages)
      : message_{message}, target_{count}, to_messages_{to_messages} {}

  bool OnMethod(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  }

  bool OnUrl(HttpMessageStreamRewriter* rewriter) override {
    ADD_FAILURE();
    return false;
  };

  bool OnVersion(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.version_);
    rewriter->RewriteCurrentToken(
        to_messages_[count_ % to_messages_.size()].version_);
    os_ << to_messages_[count_ % to_messages_.size()].version_ << " ";
    return true;
  };

  bool OnStatus(HttpMessageStreamRewriter* rewriter) override {
    EXPECT_EQ(rewriter->CurrentToken(), message_.status_);
    rewriter->RewriteCurrentToken(
        to_messages_[count_ % to_messages_.size()].status_);
    os_ << to_messages_[count_ % to_messages_.size()].status_ << "\r\n";
    return true;
  };

  bool OnHeaderPair(HttpMessageStreamRewriter* rewriter) override {
    if (!message_.headers_.size()) {
      ADD_FAILURE();
      return false;
    }

    size_t index = origin_header_count_;
    EXPECT_EQ(rewriter->CurrentToken(), message_.headers_[index].first + ": " +
                                            message_.headers_[index].second);
    EXPECT_EQ(rewriter->CurrentHeader(), message_.headers_[index]);
    origin_header_count_++;

    if (boost::iequals(rewriter->CurrentHeader().first, "Content-Length")) {
      os_ << rewriter->CurrentToken() << "\r\n";
      carryover_length_ = rewriter->CurrentToken().size() + 2;
      return true;
    }

    size_t rindex = rewrite_header_count_;
    if (to_messages_[count_ % to_messages_.size()].headers_.size() > rindex) {
      rewriter->RewriteCurrentHeader(
          to_messages_[count_ % to_messages_.size()].headers_[rindex]);
      os_ << to_messages_[count_ % to_messages_.size()].headers_[rindex].first
          << ": "
          << to_messages_[count_ % to_messages_.size()].headers_[rindex].second
          << "\r\n";
    } else {
      rewriter->DeleteCurrentHeader();
    }

    rewrite_header_count_++;
    return true;
  };

  bool OnHeaderComplete(HttpMessageStreamRewriter* rewriter) override {
    for (size_t i = rewrite_header_count_;
         i < to_messages_[count_ % to_messages_.size()].headers_.size(); i++) {
      rewriter->AddHeader(
          to_messages_[count_ % to_messages_.size()].headers_[i]);

      os_ << to_messages_[count_ % to_messages_.size()].headers_[i].first
          << ": "
          << to_messages_[count_ % to_messages_.size()].headers_[i].second
          << "\r\n";
    }

    os_ << "\r\n";
    return true;
  }

  bool OnMessageComplete(HttpMessageStreamRewriter* rewriter, size_t offset,
                         bool upgrade) override {
    origin_header_count_ = 0;
    rewrite_header_count_ = 0;

    expect_results_.push_back(os_.str());
    os_.str(std::string());

    offsets_.push_back(offset);
    current_header_len_ += (offset - current_buffer_len_);

    std::string temp = to_messages_[count_ % to_messages_.size()].Message();

    EXPECT_EQ(current_header_len_,
              to_messages_[count_ % to_messages_.size()].Message().size() +
                  message_.body_.size() + carryover_length_);
    current_header_len_ = 0;
    current_buffer_len_ = offset;
    carryover_length_ = 0;
    count_++;

    return true;
  }

  void ReturnedBuffer(Buffer* buffer) {
    current_header_len_ += buffer->size();
    current_header_len_ -= current_buffer_len_;
    current_buffer_len_ = 0;

    size_t prev = 0;
    for (auto offset : offsets_) {
      if (offset - prev) {
        char* r = (char*)std::malloc(offset - prev);
        buffer->GetData(prev, offset - prev, r);
        real_os_ << std::string(r, offset - prev);
        rewrite_results_.push_back(real_os_.str());
        real_os_.str(std::string());
        prev = offset;
        free(r);
      } else {
        rewrite_results_.push_back(real_os_.str());
        real_os_.str(std::string());
      }
    }
    offsets_.clear();

    if (buffer->size() - prev) {
      char* r = (char*)std::malloc(buffer->size() - prev);
      buffer->GetData(prev, buffer->size() - prev, r);
      real_os_ << std::string(r, buffer->size() - prev);
      free(r);
    }
  }

  void Done() {
    EXPECT_EQ(count_, target_);
    ASSERT_EQ(rewrite_results_.size(), expect_results_.size());
    for (size_t i = 0; i < rewrite_results_.size(); i++) {
      EXPECT_TRUE(boost::istarts_with(rewrite_results_[i], expect_results_[i]));
    }
  }

  size_t origin_header_count_{0}, rewrite_header_count_{0};
  HttpResponseMessage message_;
  std::vector<HttpResponseMessage> to_messages_;
  std::vector<std::string> rewrite_results_, expect_results_;
  std::vector<size_t> offsets_;
  std::ostringstream os_, real_os_;
  size_t count_{0}, target_;
  size_t current_header_len_{0};
  size_t current_buffer_len_{0};
  size_t carryover_length_{0};
};

std::vector<HttpResponseMessage> to_response_messages = {
    {"HTTP/1.1", "200 OK?????", {{"Date-jeke", "NOW"}, {"Content", "0"}}, ""},
    {"HTTP/1.1", "325 K", {{"Date", ""}, {"Length", "0"}}, ""},
    {"HTTP/1.1",
     "200 OK",
     {{"Content-Length", "0"}, {"Date", "NOWajljaef"}},
     ""},
    {"HTTP/1.1", "200 Connection Established", {}, ""},
};

TEST(HttpMessageStreamRewriterResponseTest, RewriteOneHeader) {
  for (size_t ind = 3; ind < response_messages.size(); ind++) {
    for (size_t j = 0; j < to_response_messages.size(); j++) {
      auto& message = response_messages[ind];
      auto to_messages =
          std::vector<HttpResponseMessage>{to_response_messages[j]};
      for (size_t i = 1; i < message.Message().size(); i++) {
        SCOPED_TRACE("Response Message (" + std::to_string(ind) + ") " +
                     "Rewrite to (" + std::to_string(j) +
                     "): " + "with buffer chunk size: " + std::to_string(i));

        auto delegate =
            std::make_shared<MockHttpStreamRewriterMessageDelegate>();
        auto real_delegate =
            std::make_unique<VanillaHttpResponseMessageRewriteDelegate>(
                message, 1, to_messages);
        auto delegate_ptr = real_delegate.get();
        delegate->DelegateToFake(std::move(real_delegate), message, 1);

        auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
            HttpMessageStreamRewriter::Type::Response, delegate);
        rewriter->SetSkipBodyInResponse(true);

        auto buffer =
            HttpMessageGenerator::GetHeaderBuffer(i, message.Message());

        EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
        delegate_ptr->ReturnedBuffer(buffer.get());
        delegate_ptr->Done();
      }
    }
  }
}

TEST(HttpMessageStreamRewriterResponseTest, RewriteHeadersInStandaloneBuffer) {
  for (size_t ind = 0; ind < response_messages.size(); ind++) {
    auto& message = response_messages[ind];
    auto& to_messages = to_response_messages;
    auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
    auto real_delegate =
        std::make_unique<VanillaHttpResponseMessageRewriteDelegate>(
            message, message.Message().size(), to_messages);
    auto delegate_ptr = real_delegate.get();
    delegate->DelegateToFake(std::move(real_delegate), message,
                             message.Message().size());

    auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
        HttpMessageStreamRewriter::Type::Response, delegate);
    rewriter->SetSkipBodyInResponse(true);

    for (size_t i = 1; i <= message.Message().size(); i++) {
      SCOPED_TRACE("Response Message (" + std::to_string(ind) + ") " +
                   "with buffer chunk size: " + std::to_string(i));

      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, message.Message());
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBuffer(buffer.get());
    }
    delegate_ptr->Done();
  }
}

TEST(HttpMessageStreamRewriterResponseTest, RewriteScatteredHeader) {
  for (size_t ind = 0; ind < response_messages.size(); ind++) {
    auto& message = response_messages[ind];
    std::ostringstream os;

    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    auto& to_messages = to_response_messages;

    for (size_t i = 1; i < header.size(); i++) {
      SCOPED_TRACE("Response Message (" + std::to_string(ind) + ") " +
                   "with buffer chunk size: " + std::to_string(i));

      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpResponseMessageRewriteDelegate>(
              message, MAX_CONTINUES_MESSAGE, to_messages);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Response, delegate);
      rewriter->SetSkipBodyInResponse(true);

      auto buffer = HttpMessageGenerator::GetHeaderBuffer(i, header);
      EXPECT_TRUE(rewriter->RewriteBuffer(buffer.get()));
      delegate_ptr->ReturnedBuffer(buffer.get());
      delegate_ptr->Done();
    }
  }
}

TEST(HttpMessageStreamRewriterResponseTest,
     RewriteScatteredHeaderInBufferSequence) {
  for (size_t ind = 0; ind < response_messages.size(); ind++) {
    auto& message = response_messages[ind];
    std::ostringstream os;

    for (int i = 0; i < MAX_CONTINUES_MESSAGE; i++) {
      os << message.Message();
    }
    std::string header = os.str();
    auto& to_messages = to_response_messages;

    for (size_t i = 1; i < header.size(); i++) {
      auto delegate = std::make_shared<MockHttpStreamRewriterMessageDelegate>();
      auto real_delegate =
          std::make_unique<VanillaHttpResponseMessageRewriteDelegate>(
              message, MAX_CONTINUES_MESSAGE, to_messages);
      auto delegate_ptr = real_delegate.get();
      delegate->DelegateToFake(std::move(real_delegate), message,
                               MAX_CONTINUES_MESSAGE);

      auto rewriter = std::make_unique<HttpMessageStreamRewriter>(
          HttpMessageStreamRewriter::Type::Response, delegate);
      rewriter->SetSkipBodyInResponse(true);
      auto buffers = HttpMessageGenerator::GetHeaderBuffers(i, header);

      for (size_t j = 0; j < buffers.size(); j++) {
        SCOPED_TRACE("Response Message (" + std::to_string(ind) + ") " +
                     "with buffer chunk size: " + std::to_string(i) +
                     ", iter: " + std::to_string(j));

        EXPECT_TRUE(rewriter->RewriteBuffer(buffers[j].get()));
        delegate_ptr->ReturnedBuffer(buffers[j].get());
      }
      delegate_ptr->Done();
    }
  }
}
