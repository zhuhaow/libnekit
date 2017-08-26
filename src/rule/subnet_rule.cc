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

#include "nekit/rule/subnet_rule.h"

namespace nekit {
namespace rule {
SubnetRule::SubnetRule(
    std::shared_ptr<transport::AdapterFactoryInterface> adapter_factory)
    : adapter_factory_{adapter_factory} {}

void SubnetRule::AddSubnet(const boost::asio::ip::address &address,
                           uint prefix) {
  subnets_.emplace_back(address, prefix);
}

MatchResult SubnetRule::Match(std::shared_ptr<utils::Session> session) {
  if (session->type() == utils::Session::Type::Address) {
    return LookUp(session->address()) ? MatchResult::Match
                                      : MatchResult::NotMatch;
  } else {
    if (session->domain()->isFailed()) {
      return MatchResult::NotMatch;
    }
    if (!session->domain()->isResolved()) {
      return MatchResult::ResolveNeeded;
    }
    return LookUp(session->domain()->addresses()->front())
               ? MatchResult::Match
               : MatchResult::NotMatch;
  }
}

std::unique_ptr<transport::AdapterInterface> SubnetRule::GetAdapter(
    std::shared_ptr<utils::Session> session) {
  return adapter_factory_->Build(session);
}

bool SubnetRule::LookUp(const boost::asio::ip::address &address) {
  for (const auto &subnet : subnets_) {
    if (subnet.Contains(address)) {
      return true;
    }
  }
  return false;
}

}  // namespace rule
}  // namespace nekit
