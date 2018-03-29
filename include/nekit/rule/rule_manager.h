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
#include <system_error>
#include <vector>

#include <boost/asio.hpp>

#include "../utils/async_io_interface.h"
#include "../utils/cancelable.h"
#include "../utils/resolver_interface.h"
#include "rule_interface.h"

namespace nekit {
namespace rule {
class RuleManager final : public utils::AsyncIoInterface,
                          private utils::LifeTime {
 public:
  using EventHandler =
      std::function<void(std::shared_ptr<RuleInterface>, std::error_code)>;

  enum class ErrorCode { NoError, NoMatch };

  explicit RuleManager(boost::asio::io_context* io);

  void AppendRule(std::shared_ptr<RuleInterface> rule);

  const utils::Cancelable& Match(std::shared_ptr<utils::Session> session,
                                 EventHandler handler)
      __attribute__((warn_unused_result));

  boost::asio::io_context* io() override;

 private:
  void MatchIterator(
      std::vector<std::shared_ptr<RuleInterface>>::const_iterator iter,
      std::shared_ptr<utils::Session> session,
      std::shared_ptr<utils::Cancelable> cancelable, EventHandler handler);

  std::vector<std::shared_ptr<RuleInterface>> rules_;
  boost::asio::io_service* io_;
};

std::error_code make_error_code(RuleManager::ErrorCode ec);
}  // namespace rule
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::rule::RuleManager::ErrorCode> : true_type {};
}  // namespace std
