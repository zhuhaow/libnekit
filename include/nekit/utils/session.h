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
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <boost/asio.hpp>

#include "resolver_interface.h"

namespace nekit {
namespace utils {
struct Session {
 public:
  enum class Type { Domain, Address };

  using EventHandler = std::function<void(std::error_code)>;

  Session(std::string host, uint16_t port = 0);
  Session(boost::asio::ip::address ip, uint16_t port = 0);

  void Resolve(std::shared_ptr<ResolverInterface> resolver,
               ResolverInterface::AddressPreference preference,
               EventHandler&& handler);

  bool isAddressAvailable() const;

  // prefer ipv4 address
  const boost::asio::ip::address& GetBestAddress() const;
  const ResolveResult& resolveResult() const;

  Type type() const;
  const std::string& domain() const;
  const boost::asio::ip::address& address() const;
  uint16_t port() const;
  void setPort(uint16_t port);

 private:
  Type type_;
  std::string domain_;
  boost::asio::ip::address address_;
  uint16_t port_;

  ResolveResult resolve_result_;
};
}  // namespace utils
}  // namespace nekit
