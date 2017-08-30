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

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include "endpoint.h"

namespace nekit {
namespace utils {
struct Session {
 public:
  Session(std::string host, uint16_t port = 0);
  Session(boost::asio::ip::address ip, uint16_t port = 0);

  std::map<std::string, int>& int_cache() { return int_cache_; }
  std::map<std::string, std::string>& string_cache() { return string_cache_; }

  std::shared_ptr<Endpoint>& endpoint() { return endpoint_; }

 private:
  std::shared_ptr<Endpoint> endpoint_;

  // Keys begin with "NE" are reserved.
  std::map<std::string, int> int_cache_;
  std::map<std::string, std::string> string_cache_;
};
}  // namespace utils
}  // namespace nekit
