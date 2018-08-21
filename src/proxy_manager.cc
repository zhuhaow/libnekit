// MIT License

// Copyright (c) 2018 Zhuhao Wang

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

#include <boost/assert.hpp>

#include "nekit/proxy_manager.h"
#include "nekit/utils/log.h"

namespace nekit {
ProxyManager::ProxyManager(utils::Runloop *runloop) : runloop_{runloop} {}

void ProxyManager::SetRuleManager(
    std::unique_ptr<rule::RuleManager> &&rule_manager) {
  BOOST_ASSERT(rule_manager->GetRunloop() == GetRunloop());

  rule_manager_ = std::move(rule_manager);
}

void ProxyManager::SetResolver(
    std::unique_ptr<utils::ResolverInterface> &&resolver) {
  BOOST_ASSERT(resolver->GetRunloop() == GetRunloop());

  resolver_ = std::move(resolver);
}

void ProxyManager::AddListener(
    std::unique_ptr<transport::ListenerInterface> &&listener) {
  BOOST_ASSERT(listener->GetRunloop() == GetRunloop());

  listeners_.emplace_back(std::move(listener));
}

void ProxyManager::Run() {
  BOOST_ASSERT(rule_manager_);
  BOOST_ASSERT(resolver_);
  BOOST_ASSERT(listeners_.size());

  auto handler =
      [this](std::unique_ptr<data_flow::LocalDataFlowInterface> &&data_flow,
             std::error_code ec) {
        if (ec) {
          NEERROR << "Error happened when accepting new socket " << ec;
          exit(1);
        }

        data_flow->Session()->set_resolver(resolver_.get());

        tunnel_manager_.Build(std::move(data_flow), rule_manager_.get()).Open();
      };

  for (auto &listener : listeners_) {
    listener->Accept(handler);
  }
}

void ProxyManager::Stop() {
  resolver_->Stop();

  for (auto &listener : listeners_) {
    listener->Close();
  }
}

utils::Runloop *ProxyManager::GetRunloop() { return runloop_; }

}  // namespace nekit
