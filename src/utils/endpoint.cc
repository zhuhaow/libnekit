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

#include "nekit/utils/endpoint.h"

#include <boost/assert.hpp>

#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "Endpoint"

namespace nekit {
namespace utils {
Endpoint::Endpoint(const std::string& host, uint16_t port)
    : domain_{host}, port_{port} {
  boost::system::error_code ec;
  address_ = boost::asio::ip::address::from_string(host, ec);

  if (ec) {
    type_ = Type::Domain;
  } else {
    type_ = Type::Address;
  }
}

Endpoint::Endpoint(const boost::asio::ip::address& ip, uint16_t port)
    : type_{Type::Address},
      domain_{ip.to_string()},
      address_{ip},
      port_{port} {}

const Cancelable& Endpoint::Resolve(EventHandler handler) {
  assert(resolver_);

  if (resolved_) {
    NEDEBUG << "Already resolved, does nothing.";

    resolver_->io().post(
        [ handler, cancelable{life_time_cancelable_pointer()} ]() {
          if (cancelable->canceled()) {
            return;
          }

          handler(NEKitErrorCode::NoError);
        });
    return life_time_cancelable();
  }

  return ForceResolve(handler);
}

const Cancelable& Endpoint::ForceResolve(EventHandler handler) {
  assert(resolver_);
  assert(!resolving_);

  NETRACE << "Start resolving domain " << domain_ << ".";

  resolving_ = true;

  resolve_cancelable_ = resolver_->Resolve(
      domain_, ResolverInterface::AddressPreference::Any,
      [ this, handler, cancelable{life_time_cancelable_pointer()} ](
          std::shared_ptr<std::vector<boost::asio::ip::address>> addresses,
          std::error_code ec) {
        if (cancelable->canceled()) {
          return;
        }

        resolving_ = false;
        resolved_ = true;

        if (ec) {
          NEERROR << "Failed to resolve " << domain_ << " due to " << ec << ".";
          error_ = ec;
          resolved_addresses_ = nullptr;
          handler(ec);
          return;
        }

        NEINFO << "Successfully resolved domain " << domain_ << ".";

        resolved_addresses_ = addresses;
        handler(ec);
      });

  return life_time_cancelable();
}

void Endpoint::CancelResolve() {
  if (!resolving_) {
    NEDEBUG << "Canceling a domain not resolving, do nothing.";
    return;
  }

  resolve_cancelable_.Cancel();
}
}  // namespace utils
}  // namespace nekit
