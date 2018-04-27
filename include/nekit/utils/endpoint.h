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

#include <memory>
#include <vector>

#include <boost/asio/ip/address.hpp>
#include <boost/noncopyable.hpp>

#include "cancelable.h"
#include "ip_protocol.h"
#include "resolver_interface.h"

namespace nekit {
namespace utils {
class Endpoint : LifeTime {
 public:
  using EventHandler = std::function<void(std::error_code)>;

  enum class Type { Domain, Address };

  Endpoint(const std::string& host, uint16_t port = 0);
  Endpoint(const boost::asio::ip::address& ip, uint16_t port = 0);

  bool operator==(const std::string& rhs) const { return domain_ == rhs; }

  bool IsAddressAvailable() const {
    return type_ == Type::Address || resolved_addresses_;
  }

  bool IsResolvable() const {
    return type_ == Type::Address || !(resolved_ && !resolved_addresses_);
  }

  Type type() const { return type_; }

  // Prefer to return the domain name of the host if available.
  std::string host() const { return domain_; }

  // The result only makes sense when `IsAddressAvailable` returns `true`.
  const boost::asio::ip::address& address() const {
    if (type_ == Type::Address) {
      return address_;
    } else {
      return resolved_addresses_->front();
    }
  }

  uint16_t port() const { return port_; }
  void set_port(uint16_t port) { port_ = port; }

  IPProtocol ip_protocol() const { return ip_protocol_; }
  void set_ip_protocol(IPProtocol protocol) { ip_protocol_ = protocol; }

  // If the domain is ever resolved. The resolving may have failed.
  bool IsResolved() const { return resolved_; }

  // If the domain is resolving.
  bool IsResolving() const { return resolving_; }

  bool IsResolveFailed() const { return resolved_ && !resolved_addresses_; }

  std::error_code resolve_error() const { return error_; }

  void set_resolver(ResolverInterface* resolver) { resolver_ = resolver; }

  Cancelable Resolve(EventHandler handler)
      __attribute__((warn_unused_result));
  Cancelable ForceResolve(EventHandler handler)
      __attribute__((warn_unused_result));

  std::shared_ptr<const std::vector<boost::asio::ip::address>>
  resolved_addresses() const {
    return resolved_addresses_;
  }

  std::shared_ptr<Endpoint> Dup() const;

 private:
  Type type_;
  std::string domain_;
  boost::asio::ip::address address_;
  uint16_t port_;
  IPProtocol ip_protocol_{IPProtocol::TCP};

  std::shared_ptr<std::vector<boost::asio::ip::address>> resolved_addresses_;
  std::error_code error_;
  ResolverInterface* resolver_{nullptr};
  bool resolved_{false};
  bool resolving_{false};
  Cancelable resolve_cancelable_;
};
}  // namespace utils
}  // namespace nekit
