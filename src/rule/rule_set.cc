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

#include "nekit/rule/rule_set.h"

#include "nekit/utils/runtime.h"

namespace nekit {
namespace rule {

void RuleSet::AppendRule(std::shared_ptr<RuleInterface> rule) {
  rules_.push_back(rule);
}

utils::Cancelable& RuleSet::Match(std::shared_ptr<utils::Session> session,
                                  EventHandler&& handler) {
  std::unique_ptr<utils::Cancelable> cancelable =
      std::make_unique<utils::Cancelable>();
  auto* cancel = cancelable.get();
  MatchIterator(rules_.cbegin(), session, std::move(cancelable),
                std::move(handler));
  return *cancel;
}

void RuleSet::MatchIterator(
    std::vector<std::shared_ptr<RuleInterface>>::const_iterator iter,
    std::shared_ptr<utils::Session> session,
    std::unique_ptr<utils::Cancelable>&& cancelable, EventHandler&& handler) {
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
        session->domain()->Resolve([
          this, handler{std::move(handler)}, cancelable{std::move(cancelable)},
          session, iter
        ](std::error_code ec) mutable {
          if (ec) {
            if (cancelable->canceled()) {
              return;
            }
            handler(nullptr, ec);
            return;
          }

          MatchIterator(iter, session, std::move(cancelable),
                        std::move(handler));
        });
        return;
    }
  }

  handler(nullptr, ErrorCode::NoMatch);
}

namespace {
struct RuleSetErrorCategory : std::error_category {
  const char* name() const noexcept override { return "Rule set"; }

  std::string message(int error_code) const override {
    switch (static_cast<RuleSet::ErrorCode>(error_code)) {
      case RuleSet::ErrorCode::NoError:
        return "no error";
      case RuleSet::ErrorCode::NoMatch:
        return "there is no rule match";
    }
  }
};

const RuleSetErrorCategory ruleSetErrorCategory{};
}  // namespace

std::error_code make_error_code(RuleSet::ErrorCode ec) {
  return {static_cast<int>(ec), ruleSetErrorCategory};
}
}  // namespace rule
}  // namespace nekit
