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

std::string RuleManagerErrorCategory::Description(
    const utils::Error& error) const {
  (void)error;
  return "no match";
}

std::string RuleManagerErrorCategory::DebugDescription(
    const utils::Error& error) const {
  return Description(error);
}

RuleManager::RuleManager(utils::Runloop* runloop) : runloop_{runloop} {}

RuleManager::~RuleManager() { lifetime_.Cancel(); }

void RuleManager::AppendRule(std::shared_ptr<RuleInterface> rule) {
  rules_.push_back(rule);
}

utils::Cancelable RuleManager::Match(std::shared_ptr<utils::Session> session,
                                     EventHandler handler) {
  auto cancelable = utils::Cancelable();
  runloop_->Post([this, session, cancelable, lifetime{lifetime_}, handler]() {
    if (cancelable.canceled() || lifetime.canceled()) {
      return;
    }

    MatchIterator(rules_.cbegin(), session, cancelable, handler);
  });

  return cancelable;
}

void RuleManager::MatchIterator(
    std::vector<std::shared_ptr<RuleInterface>>::const_iterator iter,
    std::shared_ptr<utils::Session> session, utils::Cancelable cancelable,
    EventHandler handler) {
  if (cancelable.canceled()) {
    return;
  }

  while (iter != rules_.cend()) {
    switch ((*iter)->Match(session)) {
      case MatchResult::Match:
        handler(*iter);
        return;
      case MatchResult::NotMatch:
        iter++;
        break;
      case MatchResult::ResolveNeeded: {
        // The lifetime of callback block is already bound to the caller of
        // `Match` and `this`. There is no need to guard the lifetime of the
        // callback in another `Cancelable`.
        (void)session->endpoint()->Resolve(
            [this, handler, cancelable, lifetime{lifetime_}, session,
             iter](utils::Result<void>&& result) mutable {
              // Resolve failure should be handled by rules.
              (void)result;

              if (cancelable.canceled() || lifetime.canceled()) {
                return;
              }

              MatchIterator(iter, session, cancelable, handler);
            });
        return;
      }
    }
  }

  handler(utils::MakeErrorResult(
      utils::Error(RuleManagerErrorCategory::GlobalRuleManagerErrorCategory(),
                   (int)RuleManagerErrorCode::NoMatch)));
}

utils::Runloop* RuleManager::GetRunloop() { return runloop_; }

}  // namespace rule
}  // namespace nekit
