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

#include "nekit/utils/system_resolver.h"

#include "nekit/utils/boost_error.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "System resolver"

using namespace std;

namespace nekit {
namespace utils {
SystemResolver::SystemResolver(boost::asio::io_service& io) : resolver_{io} {}

void SystemResolver::Resolve(std::string domain, AddressPreference preference,
                             EventHandler&& handler) {
  // Preference is ignored. Let the OS do the choice.
  (void)preference;

  decltype(resolver_)::query query(domain, "");

  NEDEBUG << "Start resolving " << domain << ".";

  resolver_.async_resolve(
      query, [ this, domain, handler{std::move(handler)} ](
                 const boost::system::error_code& ec,
                 boost::asio::ip::tcp::resolver::iterator iter) {
        if (ec) {
          auto error = ConvertBoostError(ec);
          NEERROR << "Failed to resolve " << domain << " due to " << error
                  << ".";
          handler(nullptr, error);
          return;
        }

        auto addresses =
            std::make_shared<std::vector<boost::asio::ip::address>>();
        while (iter != decltype(iter)()) {
          addresses->emplace_back(iter->endpoint().address());
          iter++;
        }

        NEINFO << "Successfully resolved domain " << domain << ".";

        handler(addresses, NEKitErrorCode::NoError);
        return;
      });
}

void SystemResolver::Cancel() { resolver_.cancel(); }

std::error_code SystemResolver::ConvertBoostError(
    const boost::system::error_code& ec) {
  if (ec.category() == boost::asio::error::system_category) {
    switch (ec.value()) {
      case boost::asio::error::basic_errors::operation_aborted:
        return NEKitErrorCode::Canceled;
    }
  }
  return std::make_error_code(ec);
}

SystemResolverFactory::SystemResolverFactory(boost::asio::io_service& io)
    : io_{&io} {}

std::unique_ptr<ResolverInterface> SystemResolverFactory::Build() {
  return std::make_unique<SystemResolver>(*io_);
}
}  // namespace utils
}  // namespace nekit
