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

#include <cstring>

#include <boost/assert.hpp>

#include "nekit/utils/subnet.h"

namespace nekit {
namespace utils {
Subnet::Subnet(const boost::asio::ip::address& address, int prefix)
    : is_ipv4_{address.is_v4()}, prefix_{prefix} {
  BOOST_ASSERT(prefix);

  if (is_ipv4_) {
    BOOST_ASSERT(prefix <= 32);
    mask_data_ = std::make_unique<uint32_t[]>(1);
    uint32_t mask = ~uint32_t(0);
    if (prefix_ < 32) {
      mask <<= 32 - prefix_;
    }
    *mask_data_.get() = htonl(mask);

    network_address_data_ = std::make_unique<uint32_t[]>(1);
    memcpy(network_address_data_.get(), address.to_v4().to_bytes().data(), 4);
    *network_address_data_.get() &= *mask_data_.get();
  } else {
    BOOST_ASSERT(prefix <= 128);
    network_address_data_ = std::make_unique<uint32_t[]>(4);
    memcpy(network_address_data_.get(), address.to_v6().to_bytes().data(), 16);

    mask_data_ = std::make_unique<uint32_t[]>(4);

    int remain = prefix_;
    uint32_t* mask = mask_data_.get();
    uint32_t* network = network_address_data_.get();
    int n = 4;
    // Note the data block is zero initialized.
    while (remain) {
      if (remain >= 32) {
        *mask++ = ~uint32_t(0);
        remain -= 32;
        network++;
      } else {
        *mask = htonl(~uint32_t(0) << (32 - remain));
        *network++ &= *mask;
        remain = 0;
      }
      n--;
    }

    while (n--) {
      *network++ = 0;
    }
  }
}

bool Subnet::Contains(const boost::asio::ip::address& address) const {
  if (address.is_v4() != is_ipv4_) {
    return false;
  }

  if (is_ipv4_) {
    return (*reinterpret_cast<uint32_t*>(address.to_v4().to_bytes().data()) &
            *mask_data_.get()) == *network_address_data_.get();
  } else {
    int n = 4;
    uint32_t* mask = mask_data_.get();
    uint32_t* network = network_address_data_.get();
    uint32_t* addr =
        reinterpret_cast<uint32_t*>(address.to_v6().to_bytes().data());
    while (n--) {
      if ((*mask++ & *addr++) != *network++) {
        return false;
      }
    }
    return true;
  }
}
}  // namespace utils
}  // namespace nekit
