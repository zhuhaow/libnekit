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

#include "nekit/rule/geo_rule.h"

#include <boost/assert.hpp>

namespace nekit {
namespace rule {

namespace {
const std::string CountryIsoCodeCacheKey = "NECC";
}

GeoRule::GeoRule(utils::CountryIsoCode code, bool match, RuleHandler handler)
    : code_{code}, match_{match}, handler_{handler} {}

MatchResult GeoRule::Match(std::shared_ptr<utils::Session> session) {
  BOOST_ASSERT(session->endpoint());

  utils::CountryIsoCode code;

  auto iter = session->int_cache().find(CountryIsoCodeCacheKey);
  if (iter != session->int_cache().end()) {
    code = static_cast<utils::CountryIsoCode>(iter->second);
  } else {
    if (session->endpoint()->IsAddressAvailable()) {
      code = LookupAndCache(session, session->endpoint()->address());
    } else {
      if (session->endpoint()->IsResolvable()) {
        return MatchResult::ResolveNeeded;
      } else {
        return MatchResult::NotMatch;
      }
    }
  }

  if ((code == code_) == match_) {
    return MatchResult::Match;
  } else {
    return MatchResult::NotMatch;
  }
}  // namespace rule

std::unique_ptr<data_flow::RemoteDataFlowInterface> GeoRule::GetDataFlow(
    std::shared_ptr<utils::Session> session) {
  BOOST_ASSERT(session->endpoint());

  return handler_(session);
}

utils::CountryIsoCode GeoRule::LookupAndCache(
    std::shared_ptr<utils::Session> session,
    const boost::asio::ip::address &address) {
  auto geo_result = utils::Maxmind::Lookup(address);
  auto code = geo_result->country_iso_code();
  session->int_cache()[CountryIsoCodeCacheKey] = static_cast<int>(code);
  return code;
}
}  // namespace rule
}  // namespace nekit
