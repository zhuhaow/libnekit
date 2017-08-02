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
class Domain final {
 public:
  using EventHandler = std::function<void(std::error_code)>;

  Domain(std::string domain, std::unique_ptr<ResolverInterface>&& resolver);

  bool operator==(const std::string& rhs) const;

  void Resolve(EventHandler&& handler);
  void ForceResolve(EventHandler&& handler);
  void Cancel();

  // If the domain is ever resolved. The resolving may fail.
  bool isResolved() const;
  // If the domain is resolving.
  bool isResolving() const;
  // If the domain is resolved and there is an address available to connect.
  bool isAddressAvailable() const;

  // If the domain is resolved but the resolving failed.
  bool isFailed() const;
  std::error_code error() const;

  std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses()
      const;

 private:
  std::string domain_;
  std::unique_ptr<ResolverInterface> resolver_;

  std::shared_ptr<std::vector<boost::asio::ip::address>> addresses_;
  std::error_code error_;
  bool resolved_{false};
  bool resolving_{false};
};
}  // namespace utils
}  // namespace nekit
