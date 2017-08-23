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

#include "nekit/rule/domain_regex_rule.h"

namespace nekit {
namespace rule {
DomainRegexRule::DomainRegexRule(
    std::shared_ptr<transport::AdapterFactoryInterface> adapter_factory)
    : adapter_factory_{adapter_factory} {}

bool DomainRegexRule::AddRegex(const std::string &expression) {
  try {
    regex_list_.emplace_back(expression,
                             std::regex::ECMAScript | std::regex::nosubs |
                                 std::regex::icase | std::regex::optimize);
  } catch (...) {
    return false;
  }
  return true;
}

MatchResult DomainRegexRule::Match(std::shared_ptr<utils::Session> session) {
  if (session->type() == utils::Session::Type::Address) {
    return MatchResult::NotMatch;
  }

  const std::string &domain = session->domain()->domain();
  for (const auto &regex : regex_list_) {
    if (std::regex_search(domain, regex)) {
      return MatchResult::Match;
    }
  }

  return MatchResult::NotMatch;
}

std::unique_ptr<transport::AdapterInterface> DomainRegexRule::GetAdapter(
    std::shared_ptr<utils::Session> session) {
  return adapter_factory_->Build(session);
}
}  // namespace rule
}  // namespace nekit
