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

#include "nekit/rule/all_rule.h"

namespace nekit {
namespace rule {
AllRule::AllRule(
    std::unique_ptr<stream_coder::StreamCoderFactoryInterface>&&
        stream_coder_factory,
    std::unique_ptr<transport::ConnectorFactoryInterface>&& connector_factory,
    bool address_required)
    : stream_coder_factory_{std::move(stream_coder_factory)},
      connector_factory_{std::move(connector_factory)},
      address_required_{address_required} {}

MatchResult AllRule::Match(std::shared_ptr<utils::Session> session) {
  if (!address_required_) {
    return MatchResult::Match;
  }

  if (session->isAddressAvailable()) {
    return MatchResult::Match;
  } else if (session->isResolved()) {
    return MatchResult::NotMatch;
  }
  return MatchResult::ResolveNeeded;
}

std::unique_ptr<transport::ConnectorInterface> AllRule::GetConnector(
    std::shared_ptr<const utils::Session> session) {
  return connector_factory_->Build(*session);
}

std::unique_ptr<stream_coder::StreamCoderInterface>
AllRule::AllRule::GetStreamCoder(
    std::shared_ptr<const utils::Session> session) {
  return stream_coder_factory_->Build(*session);
}

}  // namespace rule
}  // namespace nekit
