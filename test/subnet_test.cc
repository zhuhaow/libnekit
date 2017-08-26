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

#include <boost/asio/ip/address.hpp>

#include "nekit/utils/subnet.h"

using namespace nekit::utils;
using namespace boost::asio::ip;

TEST(SubnetUnitTest, Ipv4AddressTest) {
  Subnet subnet{address::from_string("127.0.0.1"), 32};
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.0.1")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.0.0")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.0.2")));
  ASSERT_FALSE(subnet.Contains(address::from_string("fe80::1")));
}

TEST(SubnetUnitTest, Ipv4SubnetTest) {
  Subnet subnet{address::from_string("127.0.0.1"), 20};
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.0.0")));
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.0.1")));
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.0.2")));
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.1.1")));
  ASSERT_TRUE(subnet.Contains(address::from_string("127.0.15.1")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.16.1")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.1.0.1")));
  ASSERT_FALSE(subnet.Contains(address::from_string("fe80::1")));
}

TEST(SubnetUnitTest, Ipv6AddressTest) {
  Subnet subnet{address::from_string("fe80::"), 128};
  ASSERT_TRUE(subnet.Contains(address::from_string("fe80::")));
  ASSERT_FALSE(subnet.Contains(address::from_string("fe80::1")));
  ASSERT_FALSE(subnet.Contains(address::from_string("::")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.0.0")));
}

TEST(SubnetUnitTest, Ipv6SubnetTest) {
  Subnet subnet{address::from_string("fe80::"), 60};
  ASSERT_TRUE(subnet.Contains(address::from_string("fe80::")));
  ASSERT_TRUE(subnet.Contains(address::from_string("fe80::1")));
  ASSERT_TRUE(
      subnet.Contains(address::from_string("fe80::ffff:ffff:ffff:ffff")));
  ASSERT_TRUE(
      subnet.Contains(address::from_string("fe80::f:ffff:ffff:ffff:ffff")));
  ASSERT_FALSE(
      subnet.Contains(address::from_string("fe80::1f:ffff:ffff:ffff:ffff")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.0.1")));
}

TEST(SubnetUnitTest, Ipv6AlignedSubnetTest) {
  Subnet subnet{address::from_string("fe80::"), 64};
  ASSERT_TRUE(subnet.Contains(address::from_string("fe80::")));
  ASSERT_TRUE(subnet.Contains(address::from_string("fe80::1")));
  ASSERT_TRUE(
      subnet.Contains(address::from_string("fe80::ffff:ffff:ffff:ffff")));
  ASSERT_FALSE(
      subnet.Contains(address::from_string("fe80::f:ffff:ffff:ffff:ffff")));
  ASSERT_FALSE(
      subnet.Contains(address::from_string("fe80::1f:ffff:ffff:ffff:ffff")));
  ASSERT_FALSE(subnet.Contains(address::from_string("127.0.0.1")));
}
