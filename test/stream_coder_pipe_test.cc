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

#include <nekit/deps/easylogging++.h>

#include <mock/stream_coder.h>
#include <nekit/stream_coder/stream_coder_pipe.h>

INITIALIZE_EASYLOGGINGPP

using namespace nekit::stream_coder;

using ::testing::Return;
using ::testing::_;

TEST(StreamCoderPipeUnitTest, ComputesReserveSize) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kReady);
  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);
}

TEST(StreamCoderPipeNegotationTest, RemoveFirstCoder) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kRemoveSelf));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kReady);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 6);
  EXPECT_EQ(rsize.suffix(), 6);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 6);
  EXPECT_EQ(rsize.suffix(), 6);
}

TEST(StreamCoderPipeNegotationTest, RemoveMiddleCoder) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kRemoveSelf));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kReady);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 10);
  EXPECT_EQ(rsize.suffix(), 10);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 10);
  EXPECT_EQ(rsize.suffix(), 10);
}

TEST(StreamCoderPipeNegotationTest, RemoveLastCoder) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kRemoveSelf));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kReady);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);
}

TEST(StreamCoderPipeNegotationTest, FirstWantToRead) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Input(_)).WillOnce(Return(kReady));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kWantRead));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kWantRead);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 8);
  EXPECT_EQ(rsize.suffix(), 8);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 8);
  EXPECT_EQ(rsize.suffix(), 8);

  nekit::utils::Buffer buffer(nullptr, 0);
  EXPECT_EQ(pipe.Input(buffer), kReady);

  rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);
}

TEST(StreamCoderPipeNegotationTest, MiddleWantToRead) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Input(_)).WillOnce(Return(kContinue));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Input(_)).WillOnce(Return(kReady));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kWantRead));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kWantRead);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);

  nekit::utils::Buffer buffer(nullptr, 0);
  EXPECT_EQ(pipe.Input(buffer), kReady);

  rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);
}

TEST(StreamCoderPipeNegotationTest, MiddleWantToReadThenFirstRemove) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Input(_)).WillOnce(Return(kRemoveSelf));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Input(_)).WillOnce(Return(kReady));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kWantRead));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kReady));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kWantRead);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 12);
  EXPECT_EQ(rsize.suffix(), 12);

  nekit::utils::Buffer buffer(nullptr, 0);
  EXPECT_EQ(pipe.Input(buffer), kReady);

  rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 6);
  EXPECT_EQ(rsize.suffix(), 6);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 6);
  EXPECT_EQ(rsize.suffix(), 6);
}

TEST(StreamCoderPipeNegotationTest, LastWantToRead) {
  std::unique_ptr<MockStreamCoder> coder1(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder2(new MockStreamCoder());
  std::unique_ptr<MockStreamCoder> coder3(new MockStreamCoder());

  EXPECT_CALL(*coder1, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(8, 8)));
  EXPECT_CALL(*coder1, Input(_)).WillOnce(Return(kContinue));
  EXPECT_CALL(*coder1, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder2, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(4, 4)));
  EXPECT_CALL(*coder2, Input(_)).WillOnce(Return(kContinue));
  EXPECT_CALL(*coder2, Negotiate()).WillOnce(Return(kReady));

  EXPECT_CALL(*coder3, OutputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, InputReserve())
      .WillRepeatedly(Return(BufferReserveSize(2, 2)));
  EXPECT_CALL(*coder3, Input(_)).WillOnce(Return(kReady));
  EXPECT_CALL(*coder3, Negotiate()).WillOnce(Return(kWantRead));

  StreamCoderPipe pipe;
  pipe.AppendStreamCoder(std::move(coder1));
  pipe.AppendStreamCoder(std::move(coder2));
  pipe.AppendStreamCoder(std::move(coder3));

  EXPECT_EQ(pipe.Negotiate(), kWantRead);

  BufferReserveSize rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  nekit::utils::Buffer buffer(nullptr, 0);
  EXPECT_EQ(pipe.Input(buffer), kReady);

  rsize = pipe.InputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);

  rsize = pipe.OutputReserve();
  EXPECT_EQ(rsize.prefix(), 14);
  EXPECT_EQ(rsize.suffix(), 14);
}
