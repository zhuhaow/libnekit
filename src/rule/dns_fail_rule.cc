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

#include "nekit/rule/dns_fail_rule.h"

#include <boost/assert.hpp>

namespace nekit {
namespace rule {
DnsFailRule::DnsFailRule(RuleHandler handler) : handler_{handler} {}

MatchResult DnsFailRule::Match(std::shared_ptr<utils::Session> session) {
  BOOST_ASSERT(session->endpoint());

  if (session->endpoint()->IsAddressAvailable()) {
    return MatchResult::NotMatch;
  }

  if (session->endpoint()->IsResolvable()) {
    return MatchResult::ResolveNeeded;
  }

  return MatchResult::Match;
}

std::unique_ptr<data_flow::RemoteDataFlowInterface> DnsFailRule::GetDataFlow(
    std::shared_ptr<utils::Session> session) {
  return handler_(session);
}

}  // namespace rule
}  // namespace nekit
