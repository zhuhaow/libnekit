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

#include <system_error>

#include "nekit/utils/session.h"

namespace nekit {
namespace utils {

Session::Session(std::string host, uint16_t port)
    : domain_{std::make_shared<Domain>(host)}, port_{port} {
  boost::system::error_code ec;
  address_ = boost::asio::ip::address::from_string(host, ec);

  if (ec) {
    type_ = Type::Domain;
  } else {
    type_ = Type::Address;
  }
}

Session::Session(boost::asio::ip::address ip, uint16_t port)
    : type_{Type::Address},
      domain_{std::make_shared<Domain>(ip.to_string())},
      address_{ip},
      port_{port} {}

bool Session::isAddressAvailable() const {
  if (type_ == Type::Address) {
    return true;
  }

  return domain_->isAddressAvailable();
}

const boost::asio::ip::address& Session::GetBestAddress() const {
  assert(isAddressAvailable());

  if (type_ == Type::Address) {
    return address_;
  }

  return domain_->addresses()->front();
}

Session::Type Session::type() const { return type_; }

std::shared_ptr<Domain> Session::domain() { return domain_; }

const boost::asio::ip::address& Session::address() const { return address_; }

uint16_t Session::port() const { return port_; }

void Session::setPort(uint16_t port) { port_ = port; }

IPProtocol Session::ipProtocol() const { return ip_protocol_; }

std::map<std::string, int>& Session::int_cache() { return int_cache_; }

std::map<std::string, std::string>& Session::string_cache() {
  return string_cache_;
}
}  // namespace utils
}  // namespace nekit
