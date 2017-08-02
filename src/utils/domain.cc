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

#include "nekit/utils/domain.h"

#include "nekit/utils/error.h"
#include "nekit/utils/log.h"
#include "nekit/utils/runtime.h"

#undef NECHANNEL
#define NECHANNEL "Domain"

namespace nekit {
namespace utils {
Domain::Domain(std::string domain,
               std::unique_ptr<ResolverInterface>&& resolver)
    : domain_{domain}, resolver_{std::move(resolver)} {}

bool Domain::operator==(const std::string& rhs) const { return domain_ == rhs; }

void Domain::Resolve(EventHandler&& handler) {
  if (resolved_) {
    NEDEBUG << "Already resolved, does nothing.";
    Runtime::CurrentRuntime().IoService()->post([handler{
        std::move(handler)}]() { handler(NEKitErrorCode::NoError); });
    return;
  }

  ForceResolve(std::move(handler));
}

void Domain::ForceResolve(EventHandler&& handler) {
  assert(!resolving_);

  NEDEBUG << "Start resolving domain " << domain_ << ".";

  resolving_ = true;
  resolver_->Resolve(
      domain_, ResolverInterface::AddressPreference::Any,
      [ this, handler{std::move(handler)} ](
          std::shared_ptr<std::vector<boost::asio::ip::address>> addresses,
          std::error_code ec) {

        resolving_ = false;
        resolved_ = true;

        if (ec) {
          NEERROR << "Failed to resolve " << domain_ << " due to " << ec << ".";
          error_ = ec;
          addresses_ = nullptr;
          handler(ec);
          return;
        }

        NEINFO << "Successfully resolved domain " << domain_ << ".";

        addresses_ = addresses;
        handler(ec);
      });
}

void Domain::Cancel() {
  if (!resolving_) {
    NEDEBUG << "Canceling a domain not resolving, do nothing.";
    return;
  }

  resolver_->Cancel();
}

bool Domain::isResolved() const { return resolved_; }

bool Domain::isResolving() const { return resolving_; }

bool Domain::isFailed() const { return resolved_ && addresses_; }

bool Domain::isAddressAvailable() const {
  return addresses_ && !addresses_->empty();
}

std::error_code Domain::error() const { return error_; }

std::shared_ptr<const std::vector<boost::asio::ip::address>> Domain::addresses()
    const {
  return addresses_;
}

}  // namespace utils
}  // namespace nekit
