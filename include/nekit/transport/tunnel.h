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

#include <memory>
#include <unordered_map>

#include <boost/noncopyable.hpp>

#include "../data_flow/local_data_flow_interface.h"
#include "../data_flow/remote_data_flow_interface.h"
#include "../rule/rule_manager.h"
#include "../utils/cancelable.h"
#include "../utils/session.h"

namespace nekit {
namespace transport {

class TunnelManager;

class Tunnel final : private boost::noncopyable {
 public:
  Tunnel(std::unique_ptr<data_flow::LocalDataFlowInterface>&& local_data_flow,
         rule::RuleManager* rule_manager);
  ~Tunnel();

  void Open();

  friend class TunnelManager;

 private:
  void MatchRule();
  void ConnectToRemote();
  void FinishLocalNegotiation();
  void BeginForward();

  void ForwardLocal();
  void ForwardRemote();

  void CheckTunnelStatus();
  void ReleaseTunnel();

  void LocalReportError(std::error_code ec);

  std::unique_ptr<utils::Buffer> CreateBuffer();

  std::shared_ptr<utils::Session> session_;

  rule::RuleManager* rule_manager_;
  TunnelManager* tunnel_manager_;

  std::unique_ptr<data_flow::LocalDataFlowInterface> local_data_flow_;
  std::unique_ptr<data_flow::RemoteDataFlowInterface> remote_data_flow_;

  utils::Cancelable open_cancelable_, local_read_cancelable_,
      local_write_cancelable_, remote_read_cancelable_,
      remote_write_cancelable_, rule_cancelable_;
};

class TunnelManager final : private boost::noncopyable {
 public:
  Tunnel& Build(
      std::unique_ptr<data_flow::LocalDataFlowInterface>&& local_data_flow,
      rule::RuleManager* rule_manager);

  void CloseAll();

  friend class Tunnel;

 private:
  void NotifyClosed(Tunnel* tunnel);

  std::unordered_map<void*, std::unique_ptr<transport::Tunnel>> tunnels_;
};
}  // namespace transport
}  // namespace nekit
