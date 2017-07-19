// MIT License

// Copyright (c) 2017 Zhuhao Wang

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

#include <easylogging++.h>

#include <mock/stream_coder.h>
#include <nekit/stream_coder/stream_coder_pipe.h>

INITIALIZE_EASYLOGGINGPP

using namespace nekit::stream_coder;
using namespace nekit::utils;

using ::testing::Return;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Sequence;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::NiceMock;

const uint c1_ip = 1;  // ip: input prefix
const uint c1_is = 2;  // is: input suffix
const uint c1_op = 4;  // op: output prefix
const uint c1_os = 8;  // os: output suffix

const uint c2_ip = 16;   // ip: input prefix
const uint c2_is = 32;   // is: input suffix
const uint c2_op = 64;   // op: output prefix
const uint c2_os = 128;  // os: output suffix

const uint c3_ip = 256;   // ip: input prefix
const uint c3_is = 512;   // is: input suffix
const uint c3_op = 1024;  // op: output prefix
const uint c3_os = 2048;  // os: output suffix

enum MockError { kError = 0 };

class MockErrorCategory final : public std::error_category {
  const char* name() const BOOST_NOEXCEPT override { return ""; }

  std::string message(int error_code) const override {
    (void)error_code;
    return "";
  }
};

const MockErrorCategory mock_error_category{};

namespace std {
template <>
struct is_error_code_enum<MockError> : public true_type {};

error_code make_error_code(MockError errc) {
  return error_code(static_cast<int>(errc), mock_error_category);
}
}  // namespace std

class StreamCoderPipeDefaultFixture : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    coder1_.reset(new MockStreamCoder());
    coder2_.reset(new MockStreamCoder());
    coder3_.reset(new MockStreamCoder());

    ON_CALL(*coder1_, InputReserve())
        .WillByDefault(Return(BufferReserveSize(c1_ip, c1_is)));
    ON_CALL(*coder1_, OutputReserve())
        .WillByDefault(Return(BufferReserveSize(c1_op, c1_os)));
    ON_CALL(*coder1_, Negotiate()).WillByDefault(Return(ActionRequest::kReady));

    ON_CALL(*coder2_, InputReserve())
        .WillByDefault(Return(BufferReserveSize(c2_ip, c2_is)));
    ON_CALL(*coder2_, OutputReserve())
        .WillByDefault(Return(BufferReserveSize(c2_op, c2_os)));
    ON_CALL(*coder2_, Negotiate()).WillByDefault(Return(ActionRequest::kReady));

    ON_CALL(*coder3_, InputReserve())
        .WillByDefault(Return(BufferReserveSize(c3_ip, c3_is)));
    ON_CALL(*coder3_, OutputReserve())
        .WillByDefault(Return(BufferReserveSize(c3_op, c3_os)));
    ON_CALL(*coder3_, Negotiate()).WillByDefault(Return(ActionRequest::kReady));

    // We do not test these methods since they are const and the result is
    // already checked. So there it doesn't matter how they are called.
    EXPECT_CALL(*coder1_, InputReserve()).Times(AnyNumber());
    EXPECT_CALL(*coder1_, OutputReserve()).Times(AnyNumber());
    EXPECT_CALL(*coder2_, InputReserve()).Times(AnyNumber());
    EXPECT_CALL(*coder2_, OutputReserve()).Times(AnyNumber());
    EXPECT_CALL(*coder3_, InputReserve()).Times(AnyNumber());
    EXPECT_CALL(*coder3_, OutputReserve()).Times(AnyNumber());
  }

  void RegisterAllCoders() {
    pipe_.AppendStreamCoder(std::move(coder1_));
    pipe_.AppendStreamCoder(std::move(coder2_));
    pipe_.AppendStreamCoder(std::move(coder3_));
  }

  std::unique_ptr<MockStreamCoder> coder1_, coder2_, coder3_;

  StreamCoderPipe pipe_;
};

TEST(StreamCoderPipeUnitTest, ReturnErrorOnNegotiationWhenEmpty) {
  StreamCoderPipe pipe;
  EXPECT_EQ(pipe.Negotiate(), ActionRequest::kErrorHappened);
  EXPECT_EQ(pipe.GetLatestError().category(),
            StreamCoderPipe::error_category());
  EXPECT_EQ(pipe.GetLatestError().value(),
            static_cast<int>(StreamCoderPipe::kNoCoder));
  EXPECT_FALSE(pipe.forwarding());
}

TEST_F(StreamCoderPipeDefaultFixture, ComputesReserveSize) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, RemoveFirstWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  Sequence s1, s2;
  EXPECT_CALL(*coder1_, Die()).InSequence(s1, s2);
  EXPECT_CALL(*coder2_, Die()).InSequence(s1);
  EXPECT_CALL(*coder3_, Die()).InSequence(s2);

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, RemoveMiddleWhenNegotiating) {
  EXPECT_CALL(*coder2_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  Sequence s1, s2;
  EXPECT_CALL(*coder2_, Die()).InSequence(s1, s2);
  EXPECT_CALL(*coder1_, Die()).InSequence(s1);
  EXPECT_CALL(*coder3_, Die()).InSequence(s2);

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, RemoveLastWhenNegotiating) {
  EXPECT_CALL(*coder3_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate());

  Sequence s1, s2;
  EXPECT_CALL(*coder3_, Die()).InSequence(s1, s2);
  EXPECT_CALL(*coder1_, Die()).InSequence(s1);
  EXPECT_CALL(*coder2_, Die()).InSequence(s2);

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os);
}

TEST_F(StreamCoderPipeDefaultFixture, RemoveAllWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));
  EXPECT_CALL(*coder2_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));
  EXPECT_CALL(*coder3_, Negotiate())
      .WillOnce(Return(ActionRequest::kRemoveSelf));

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kErrorHappened);

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.GetLatestError().category(),
            StreamCoderPipe::error_category());
  EXPECT_EQ(pipe_.GetLatestError().value(),
            static_cast<int>(StreamCoderPipe::kNoCoder));
}

TEST_F(StreamCoderPipeDefaultFixture, FirstWantToReadWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));
  EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kReady));
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantRead);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip);
  EXPECT_EQ(rsize.suffix(), c1_is);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, MiddleWantToReadWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));
  EXPECT_CALL(*coder3_, Negotiate());

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder2_, Input(_)).WillOnce(Return(ActionRequest::kReady));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantRead);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, LastWantToReadWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder2_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder3_, Input(_)).WillOnce(Return(ActionRequest::kReady));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantRead);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture,
       MiddleWantToReadThenFirstRemoveWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));
  EXPECT_CALL(*coder3_, Negotiate());

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Input(_))
        .WillOnce(Return(ActionRequest::kRemoveSelf));
    EXPECT_CALL(*coder2_, Input(_)).WillOnce(Return(ActionRequest::kReady));
  }

  RegisterAllCoders();

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantRead);

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kReady);

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, MultipleWantToReadWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate()).WillOnce(Return(ActionRequest::kWantRead));

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder2_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder3_, Input(_)).WillOnce(Return(ActionRequest::kReady));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantRead);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip);
  EXPECT_EQ(rsize.suffix(), c1_is);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kWantRead);

  EXPECT_FALSE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, FirstWantToWriteWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));
  EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kReady));
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op);
  EXPECT_EQ(rsize.suffix(), c1_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, MiddleWantToWriteWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));
  EXPECT_CALL(*coder3_, Negotiate());

  {
    InSequence s;

    EXPECT_CALL(*coder2_, Output(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, LastWantToWriteWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));

  {
    InSequence s;

    EXPECT_CALL(*coder3_, Output(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder2_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture,
       MiddleWantToWriteThenFirstRemoveWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));
  EXPECT_CALL(*coder3_, Negotiate());

  {
    InSequence s;

    EXPECT_CALL(*coder2_, Output(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder1_, Output(_))
        .WillOnce(Return(ActionRequest::kRemoveSelf));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, MultipleWantToWriteWhenNegotiating) {
  EXPECT_CALL(*coder1_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate())
      .WillOnce(Return(ActionRequest::kWantWrite));

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder3_, Output(_)).WillOnce(Return(ActionRequest::kReady));
    EXPECT_CALL(*coder2_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op);
  EXPECT_EQ(rsize.suffix(), c1_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kWantWrite);

  EXPECT_FALSE(pipe_.forwarding());

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);

  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);
}

TEST_F(StreamCoderPipeDefaultFixture, ReturnErrorWhenInternalCoderReturnError) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate())
      .WillOnce(Return(ActionRequest::kErrorHappened));
  EXPECT_CALL(*coder2_, GetLatestError())
      .WillOnce(Return(std::make_error_code(kError)));

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kErrorHappened);

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.GetLatestError().category(), mock_error_category);
  EXPECT_EQ(pipe_.GetLatestError().value(), static_cast<int>(kError));
}

TEST_F(StreamCoderPipeDefaultFixture, Forwarding) {
  EXPECT_CALL(*coder1_, Negotiate());
  EXPECT_CALL(*coder2_, Negotiate());
  EXPECT_CALL(*coder3_, Negotiate());

  {
    InSequence s;

    EXPECT_CALL(*coder3_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder2_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder1_, Output(_)).WillOnce(Return(ActionRequest::kContinue));
  }

  {
    InSequence s;

    EXPECT_CALL(*coder1_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder2_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
    EXPECT_CALL(*coder3_, Input(_)).WillOnce(Return(ActionRequest::kContinue));
  }

  RegisterAllCoders();

  EXPECT_FALSE(pipe_.forwarding());

  EXPECT_EQ(pipe_.Negotiate(), ActionRequest::kReady);

  EXPECT_TRUE(pipe_.forwarding());

  BufferReserveSize rsize = pipe_.InputReserve();
  EXPECT_EQ(rsize.prefix(), c1_ip + c2_ip + c3_ip);
  EXPECT_EQ(rsize.suffix(), c1_is + c2_is + c3_is);

  rsize = pipe_.OutputReserve();
  EXPECT_EQ(rsize.prefix(), c1_op + c2_op + c3_op);
  EXPECT_EQ(rsize.suffix(), c1_os + c2_os + c3_os);

  nekit::utils::Buffer buffer(10);
  EXPECT_EQ(pipe_.Input(&buffer), ActionRequest::kContinue);
  EXPECT_EQ(pipe_.Output(&buffer), ActionRequest::kContinue);
}
