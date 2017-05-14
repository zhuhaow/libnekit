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

#include <nekit/utils/buffer.h>

TEST(BufferTest, UnitTest) {
  nekit::utils::Buffer buffer(10);

  EXPECT_EQ(buffer.size(), 10);
  EXPECT_EQ(buffer.capacity(), 10);
  EXPECT_EQ(buffer.data(), buffer.buffer());

  EXPECT_TRUE(buffer.ReleaseFront(0));
  EXPECT_FALSE(buffer.ReleaseFront(1));
  EXPECT_TRUE(buffer.ReleaseBack(0));
  EXPECT_FALSE(buffer.ReleaseBack(1));
  EXPECT_EQ(buffer.capacity(), 10);
  EXPECT_EQ(buffer.data(), buffer.buffer());

  EXPECT_FALSE(buffer.ReserveFront(11));
  EXPECT_TRUE(buffer.ReserveFront(3));
  EXPECT_EQ(buffer.capacity(), 7);
  EXPECT_EQ(static_cast<char *>(buffer.data()) + 3, buffer.buffer());

  EXPECT_FALSE(buffer.ReleaseFront(4));
  EXPECT_TRUE(buffer.ReleaseFront(3));
  EXPECT_EQ(buffer.data(), buffer.buffer());

  EXPECT_FALSE(buffer.ReserveBack(11));
  EXPECT_TRUE(buffer.ReserveBack(3));
  EXPECT_EQ(buffer.capacity(), 7);
  EXPECT_EQ(buffer.data(), buffer.buffer());

  EXPECT_FALSE(buffer.ReleaseBack(4));
  EXPECT_TRUE(buffer.ReleaseBack(3));
  EXPECT_EQ(buffer.data(), buffer.buffer());

  EXPECT_TRUE(buffer.ReserveFront(5));
  EXPECT_FALSE(buffer.ReserveBack(6));
  EXPECT_TRUE(buffer.ReserveBack(5));
}
