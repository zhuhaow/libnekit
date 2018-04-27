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

#include "async_io_interface.h"
#include "endpoint.h"
#include "resolver_interface.h"

namespace nekit {
namespace utils {
struct Session : public AsyncIoInterface {
 public:
  Session(boost::asio::io_context* io) : io_{io} {}

  Session(boost::asio::io_context* io, std::string host, uint16_t port = 0)
      : io_{io},
        endpoint_{std::make_shared<Endpoint>(host, port)},
        current_endpoint_{endpoint_} {}

  Session(boost::asio::io_context* io, boost::asio::ip::address ip,
          uint16_t port = 0)
      : io_{io},
        endpoint_{std::make_shared<Endpoint>(ip, port)},
        current_endpoint_{endpoint_} {}

  Session(boost::asio::io_context* io, std::shared_ptr<Endpoint> endpoint)
      : io_{io}, endpoint_{endpoint}, current_endpoint_{endpoint_} {}

  std::map<std::string, int>& int_cache() { return int_cache_; }
  std::map<std::string, std::string>& string_cache() { return string_cache_; }

  std::shared_ptr<Endpoint>& endpoint() { return endpoint_; }
  void set_endpoint(std::shared_ptr<Endpoint> endpoint) {
    endpoint_ = endpoint;
    endpoint_->set_resolver(resolver_);

    if (!current_endpoint_) current_endpoint_ = endpoint_;
  }

  std::shared_ptr<Endpoint>& current_endpoint() { return current_endpoint_; }
  void set_current_endpoint(std::shared_ptr<Endpoint> endpoint) {
    current_endpoint_ = endpoint;
    current_endpoint_->set_resolver(resolver_);
  }

  void set_resolver(ResolverInterface* resolver) {
    BOOST_ASSERT(resolver->io() == io_);

    resolver_ = resolver;
    if (current_endpoint_) current_endpoint_->set_resolver(resolver);
    if (endpoint_) endpoint_->set_resolver(resolver);
  }

  boost::asio::io_context* io() override { return io_; };

 private:
  boost::asio::io_context* io_;
  std::shared_ptr<Endpoint> endpoint_;
  std::shared_ptr<Endpoint> current_endpoint_;

  ResolverInterface* resolver_{nullptr};

  // Keys begin with "NE" are reserved.
  std::map<std::string, int> int_cache_;
  std::map<std::string, std::string> string_cache_;
};
}  // namespace utils
}  // namespace nekit
