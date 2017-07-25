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

#pragma once

#include <string>
#include <vector>

#include <boost/asio.hpp>

namespace nekit {
namespace utils {
class ResolveResult final {
 public:
  ResolveResult(std::string domain) : domain_{domain} {};

  const std::string& domain() const { return domain_; }

  bool isIPv4() const { return !resolved_ipv4_addresses_.empty(); }
  std::vector<boost::asio::ip::address>& ipv4Result() {
    return resolved_ipv4_addresses_;
  }
  const std::vector<boost::asio::ip::address>& ipv4Result() const {
    return resolved_ipv4_addresses_;
  }

  bool isIPv6() const { return !resolved_ipv6_addresses_.empty(); }
  std::vector<boost::asio::ip::address>& ipv6Result() {
    return resolved_ipv6_addresses_;
  }
  const std::vector<boost::asio::ip::address>& ipv6Result() const {
    return resolved_ipv4_addresses_;
  }

 private:
  std::string domain_;
  std::vector<boost::asio::ip::address> resolved_ipv4_addresses_;
  std::vector<boost::asio::ip::address> resolved_ipv6_addresses_;
};
}  // namespace utils
}  // namespace nekit
