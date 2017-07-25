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

#include "nekit/stream_coder/socks5_server_stream_coder.h"

INITIALIZE_EASYLOGGINGPP

using namespace nekit::stream_coder;
using namespace nekit::utils;

TEST(SOCKS5StreamCoderSessionNegotiation, CorrectIPv4Request) {
  Socks5ServerStreamCoder coder{};
  EXPECT_EQ(coder.Negotiate(), ActionRequest::WantRead);

  Buffer buffer1(coder.DecodeReserve(), 3);
  auto data = static_cast<uint8_t *>(buffer1.buffer());
  *data++ = 5;
  *data++ = 1;
  *data++ = 0;
  EXPECT_EQ(coder.Decode(&buffer1), ActionRequest::WantWrite);

  Buffer buffer2(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer2), ActionRequest::WantRead);
  EXPECT_EQ(buffer2.capacity(), 2u);
  data = static_cast<uint8_t *>(buffer2.buffer());
  EXPECT_EQ(*data++, 5);
  EXPECT_EQ(*data, 0);

  Buffer buffer3(coder.DecodeReserve(), 10);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 1;  // CONNECT
  *data++ = 0;
  *data++ = 1;  // IPv4
  *data++ = 8;
  *data++ = 8;
  *data++ = 8;
  *data++ = 8;  // 8.8.8.8
  *data++ = 0;
  *data = 53;  // 53
  EXPECT_EQ(coder.Decode(&buffer3), ActionRequest::Event);
  EXPECT_EQ(coder.session()->type(), Session::Type::Address);
  EXPECT_EQ(coder.session()->address().is_v4(), true);
  EXPECT_EQ(coder.session()->address().to_string(), "8.8.8.8");
  EXPECT_EQ(coder.session()->port(), 53);

  EXPECT_EQ(coder.Continue(), ActionRequest::WantWrite);
  Buffer buffer4(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer4), ActionRequest::Ready);
  EXPECT_EQ(buffer4.capacity(), 10u);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 0;  // SUCCESS
  *data++ = 0;
  *data++ = 1;  // IPv4
}

TEST(SOCKS5StreamCoderSessionNegotiation, CorrectIPv6Request) {
  Socks5ServerStreamCoder coder{};
  EXPECT_EQ(coder.Negotiate(), ActionRequest::WantRead);

  Buffer buffer1(coder.DecodeReserve(), 3);
  auto data = static_cast<uint8_t *>(buffer1.buffer());
  *data++ = 5;
  *data++ = 1;
  *data++ = 0;
  EXPECT_EQ(coder.Decode(&buffer1), ActionRequest::WantWrite);

  Buffer buffer2(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer2), ActionRequest::WantRead);
  EXPECT_EQ(buffer2.capacity(), 2u);
  data = static_cast<uint8_t *>(buffer2.buffer());
  EXPECT_EQ(*data++, 5);
  EXPECT_EQ(*data, 0);

  Buffer buffer3(coder.DecodeReserve(), 22);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 1;  // CONNECT
  *data++ = 0;
  *data++ = 4;  // IPv6
  int n = 16;
  while (n--) {
    *data++ = 8;
  }
  *data++ = 0;
  *data = 53;  // 53
  EXPECT_EQ(coder.Decode(&buffer3), ActionRequest::Event);
  EXPECT_EQ(coder.session()->type(), Session::Type::Address);
  EXPECT_EQ(coder.session()->address().is_v6(), true);
  EXPECT_EQ(coder.session()->address().to_string(),
            "808:808:808:808:808:808:808:808");
  EXPECT_EQ(coder.session()->port(), 53);

  EXPECT_EQ(coder.Continue(), ActionRequest::WantWrite);
  Buffer buffer4(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer4), ActionRequest::Ready);
  EXPECT_EQ(buffer4.capacity(), 22u);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 0;  // SUCCESS
  *data++ = 0;
  *data++ = 4;  // IPv6
}

TEST(SOCKS5StreamCoderSessionNegotiation, CorrectDomainRequest) {
  Socks5ServerStreamCoder coder{};
  EXPECT_EQ(coder.Negotiate(), ActionRequest::WantRead);

  Buffer buffer1(coder.DecodeReserve(), 3);
  auto data = static_cast<uint8_t *>(buffer1.buffer());
  *data++ = 5;
  *data++ = 1;
  *data++ = 0;
  EXPECT_EQ(coder.Decode(&buffer1), ActionRequest::WantWrite);

  Buffer buffer2(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer2), ActionRequest::WantRead);
  EXPECT_EQ(buffer2.capacity(), 2u);
  data = static_cast<uint8_t *>(buffer2.buffer());
  EXPECT_EQ(*data++, 5);
  EXPECT_EQ(*data, 0);

  Buffer buffer3(coder.DecodeReserve(), 4 + 1 + 11 + 2);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 1;  // CONNECT
  *data++ = 0;
  *data++ = 3;  // domain
  *data++ = 11;
  std::string domain = "example.com";
  memcpy(data, domain.data(), 11);
  data += 11;
  *data++ = 0;
  *data = 53;  // 53
  EXPECT_EQ(coder.Decode(&buffer3), ActionRequest::Event);
  EXPECT_EQ(coder.session()->type(), Session::Type::Domain);
  EXPECT_EQ(coder.session()->domain(), "example.com");
  EXPECT_EQ(coder.session()->port(), 53);

  EXPECT_EQ(coder.Continue(), ActionRequest::WantWrite);
  Buffer buffer4(coder.EncodeReserve());
  EXPECT_EQ(coder.Encode(&buffer4), ActionRequest::Ready);
  EXPECT_EQ(buffer4.capacity(), 10u);
  data = static_cast<uint8_t *>(buffer3.buffer());
  *data++ = 5;
  *data++ = 0;  // SUCCESS
  *data++ = 0;
  *data++ = 1;  // IPv4
}
