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

#include "nekit/rule/rule_manager.h"

namespace nekit {
namespace rule {

RuleManager::RuleManager(boost::asio::io_service& io) : io_{&io} {}

void RuleManager::AppendRule(std::shared_ptr<RuleInterface> rule) {
  rules_.push_back(rule);
}

std::unique_ptr<utils::ResolverInterface>& RuleManager::resolver() {
  return resolver_;
}

void RuleManager::set_resolver(
    std::unique_ptr<utils::ResolverInterface> resolver) {
  resolver_ = std::move(resolver);
}

const utils::Cancelable& RuleManager::Match(
    std::shared_ptr<utils::Session> session, EventHandler handler) {
  std::shared_ptr<utils::Cancelable> cancelable =
      std::make_shared<utils::Cancelable>();
  io_->post([
    this, session, cancelable, lifetime{life_time_cancelable_pointer()}, handler
  ]() {
    if (lifetime->canceled()) {
      return;
    }

    MatchIterator(rules_.cbegin(), session, cancelable, handler);
  });
  return *cancelable;
}

void RuleManager::MatchIterator(
    std::vector<std::shared_ptr<RuleInterface>>::const_iterator iter,
    std::shared_ptr<utils::Session> session,
    std::shared_ptr<utils::Cancelable> cancelable, EventHandler handler) {
  if (cancelable->canceled()) {
    return;
  }

  while (iter != rules_.cend()) {
    switch ((*iter)->Match(session)) {
      case MatchResult::Match:
        handler(*iter, ErrorCode::NoError);
        return;
      case MatchResult::NotMatch:
        iter++;
        break;
      case MatchResult::ResolveNeeded:
        // Ideally, the returned `Cancelable` should have the same lifetime as
        // the caller, `RuleManager`. This can be achieved by saving the
        // `Cancelable` in some container (`std::unordered_map` for example).
        // However, it seems too much hassle since `RuleManager`'s lifetime is
        // already bond to the block.
        // There is no way to silence the warning in GCC.
        (void)session->endpoint()->Resolve([
          this, handler, cancelable, lifetime{life_time_cancelable_pointer()},
          session, iter
        ](std::error_code ec) mutable {
          (void)ec;

          if (cancelable->canceled() || lifetime->canceled()) {
            return;
          }

          MatchIterator(iter, session, cancelable, handler);
        });
        return;
    }
  }
  handler(nullptr, ErrorCode::NoMatch);
}

namespace {
struct RuleManagerErrorCategory : std::error_category {
  const char* name() const noexcept override { return "Rule manager"; }

  std::string message(int error_code) const override {
    switch (static_cast<RuleManager::ErrorCode>(error_code)) {
      case RuleManager::ErrorCode::NoError:
        return "no error";
      case RuleManager::ErrorCode::NoMatch:
        return "there is no rule match";
    }
  }
};

const RuleManagerErrorCategory ruleSetErrorCategory{};
}  // namespace

std::error_code make_error_code(RuleManager::ErrorCode ec) {
  return {static_cast<int>(ec), ruleSetErrorCategory};
}
}  // namespace rule
}  // namespace nekit
