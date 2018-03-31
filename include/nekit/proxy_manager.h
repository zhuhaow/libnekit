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

#pragma once

#include "transport/listener_interface.h"
#include "transport/tunnel.h"
#include "utils/async_io_interface.h"
#include "utils/resolver_interface.h"

namespace nekit {
class ProxyManager : public utils::AsyncIoInterface {
 public:
  void SetRuleManager(std::unique_ptr<rule::RuleManager> &&rule_manager);
  void SetResolver(std::unique_ptr<utils::ResolverInterface> &&resolver);
  void AddListener(std::unique_ptr<transport::ListenerInterface> &&listener);

  void Run();
  void Stop();
  void Reset();

  boost::asio::io_context *io() override;

 private:
  bool CheckOrSetIo(utils::AsyncIoInterface *io_interface);

  std::unique_ptr<rule::RuleManager> rule_manager_;
  std::unique_ptr<utils::ResolverInterface> resolver_;
  std::vector<std::unique_ptr<transport::ListenerInterface>> listeners_;
  transport::TunnelManager tunnel_manager_;

  boost::asio::io_context *io_{nullptr};
};
}  // namespace nekit
