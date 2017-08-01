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

using namespace std;

namespace nekit {
namespace utils {
SystemResolver::SystemResolver(boost::asio::io_service& io) : resolver_{io} {}

void SystemResolver::Resolve(std::string domain, AddressPreference preference,
                             EventHandler&& handler) {
  // Preference is ignored. Let the OS do the choice.
  (void)preference;

  decltype(resolver_)::query query(domain, "");
  resolver_.async_resolve(
      query, [ domain, handler{std::move(handler)} ](
                 const boost::system::error_code& ec,
                 boost::asio::ip::tcp::resolver::iterator iter) {
        if (ec) {
          handler(nullptr, std::make_error_code(ec));
          return;
        }

        auto addresses =
            std::make_shared<std::vector<boost::asio::ip::address>>();
        while (iter != decltype(iter)()) {
          addresses->emplace_back(iter->endpoint().address());
            iter++;
        }

        handler(addresses, std::make_error_code(ec));
        return;
      });
}
}  // namespace utils
}  // namespace nekit
