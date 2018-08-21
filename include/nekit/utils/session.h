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

#include "async_interface.h"
#include "endpoint.h"
#include "resolver_interface.h"

namespace nekit {
namespace utils {
struct Session : public AsyncInterface {
 public:
  Session(Runloop* runloop) : runloop_{runloop} {}

  Session(Runloop* runloop, std::string host, uint16_t port = 0)
      : runloop_{runloop}, endpoint_{std::make_shared<Endpoint>(host, port)} {}

  Session(Runloop* runloop, boost::asio::ip::address ip, uint16_t port = 0)
      : runloop_{runloop}, endpoint_{std::make_shared<Endpoint>(ip, port)} {}

  Session(Runloop* runloop, std::shared_ptr<Endpoint> endpoint)
      : runloop_{runloop}, endpoint_{endpoint} {}

  std::map<std::string, int>& int_cache() { return int_cache_; }
  std::map<std::string, std::string>& string_cache() { return string_cache_; }

  std::shared_ptr<Endpoint>& endpoint() { return endpoint_; }
  void set_endpoint(std::shared_ptr<Endpoint> endpoint) {
    endpoint_ = endpoint;
    endpoint_->set_resolver(resolver_);
  }

  void set_resolver(ResolverInterface* resolver) {
    BOOST_ASSERT(resolver->GetRunloop() == GetRunloop());

    resolver_ = resolver;
    if (endpoint_) endpoint_->set_resolver(resolver);
  }

  Runloop* GetRunloop() override { return runloop_; };

 private:
  Runloop* runloop_;
  std::shared_ptr<Endpoint> endpoint_;

  ResolverInterface* resolver_{nullptr};

  // Keys begin with "NE" are reserved.
  std::map<std::string, int> int_cache_;
  std::map<std::string, std::string> string_cache_;
};
}  // namespace utils
}  // namespace nekit
