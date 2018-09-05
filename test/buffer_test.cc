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

#include <memory>

#include <gtest/gtest.h>

#include <nekit/utils/buffer.h>

using namespace nekit;

void EvaluateBufferRange(utils::Buffer* buffer, size_t offset, size_t len,
                         size_t start) {
  for (size_t i = 0; i < len; i++) {
    EXPECT_EQ((*buffer)[offset + i], start + i);
  }
}

void FillBuffer(utils::Buffer* buffer, size_t offset, size_t len,
                size_t start) {
  for (size_t i = 0; i < len; i++) {
    (*buffer)[offset + i] = start + i;
  }
}

TEST(BufferCreateTest, ZeroSize) {
  utils::Buffer buffer{0};
  EXPECT_EQ(buffer.size(), 0);
}

TEST(BufferCreateTest, WholeBuffer) {
  utils::Buffer buffer{200};
  EXPECT_EQ(buffer.size(), 200);

  FillBuffer(&buffer, 0, 200, 0);

  EvaluateBufferRange(&buffer, 0, 200, 0);
}

TEST(BufferCreateTest, ChunkedBuffer) {
  utils::Buffer buffer{0};

  for (size_t i = 0; i < 100; i++) {
    buffer.InsertBack(2);
  }

  FillBuffer(&buffer, 0, 200, 0);

  EvaluateBufferRange(&buffer, 0, 200, 0);
}

class BufferFactory {
 public:
  static std::unique_ptr<utils::Buffer> WholeBuffer(size_t size) {
    return std::make_unique<utils::Buffer>(size);
  }

  static std::unique_ptr<utils::Buffer> ChunkedBuffer(size_t chunk_size,
                                                      size_t count) {
    auto buffer = std::make_unique<utils::Buffer>(0);
    while (count--) {
      buffer->InsertBack(chunk_size);
    }
    return buffer;
  }

  static std::unique_ptr<utils::Buffer> ZeroBuffer() {
    return std::make_unique<utils::Buffer>(0);
  }
};

TEST(BufferInsertTest, CheckInsert) {
  auto buffer = BufferFactory::WholeBuffer(30);

  FillBuffer(buffer.get(), 0, 30, 0);
  buffer->Insert(1, 1);
  EXPECT_EQ(buffer->size(), 31);
  EvaluateBufferRange(buffer.get(), 0, 1, 0);
  EvaluateBufferRange(buffer.get(), 2, 29, 1);
}

TEST(BufferInsertTest, CheckInsertOnChunkedBuffer) {
  auto buffer = BufferFactory::ChunkedBuffer(2, 15);

  FillBuffer(buffer.get(), 0, 30, 0);
  buffer->Insert(1, 1);
  EXPECT_EQ(buffer->size(), 31);
  EvaluateBufferRange(buffer.get(), 0, 1, 0);
  EvaluateBufferRange(buffer.get(), 2, 29, 1);
}

TEST(BufferBreakTest, CheckBreak) {
  auto buffer = BufferFactory::WholeBuffer(30);
  FillBuffer(buffer.get(), 0, 30, 0);
  auto b = buffer->Break(20);
  EvaluateBufferRange(buffer.get(), 0, 20, 0);
  EvaluateBufferRange(&b, 0, 10, 20);
}

TEST(BufferBreakTest, CheckChunkedBufferBreak) {
  auto buffer = BufferFactory::ChunkedBuffer(10, 3);
  FillBuffer(buffer.get(), 0, 30, 0);
  auto b = buffer->Break(20);
  EvaluateBufferRange(buffer.get(), 0, 20, 0);
  EvaluateBufferRange(&b, 0, 10, 20);
}
