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

#pragma once

#include <vector>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "rule/rule_manager.h"
#include "transport/server_listener_interface.h"
#include "transport/tunnel.h"
#include "utils/async_io_interface.h"
#include "utils/resolver_interface.h"

namespace nekit {
class Instance : public utils::AsyncIoInterface, private boost::noncopyable {
 public:
  explicit Instance(std::string name);

  void SetRuleManager(std::unique_ptr<rule::RuleManager> &&rule_manager);
  void AddListener(
      std::unique_ptr<transport::ServerListenerInterface> &&listener);

  void Run();
  void Stop();
  void ResetNetwork();

 private:
  std::string name_;
  boost::asio::io_service io_;

  std::unique_ptr<rule::RuleManager> rule_manager_;
  std::vector<std::unique_ptr<transport::ServerListenerInterface>> listeners_;
  transport::TunnelManager tunnel_manager_;

  bool ready_{true};
};
}  // namespace nekit
