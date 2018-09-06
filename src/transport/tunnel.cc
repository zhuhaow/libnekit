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

#include "nekit/transport/tunnel.h"

#include "nekit/config.h"
#include "nekit/utils/common_error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "Tunnel"

#define TIMEOUT_INTERVAL 300 * 1000

namespace nekit {
namespace transport {

Tunnel::Tunnel(
    std::unique_ptr<data_flow::LocalDataFlowInterface>&& local_data_flow,
    rule::RuleManager* rule_manager)
    : session_{local_data_flow->Session()},
      rule_manager_{rule_manager},
      local_data_flow_{std::move(local_data_flow)},
      timeout_timer_{session_->GetRunloop(), [this]() { ReleaseTunnel(); }} {}

Tunnel::~Tunnel() {
  open_cancelable_.Cancel();
  local_read_cancelable_.Cancel();
  local_write_cancelable_.Cancel();
  remote_read_cancelable_.Cancel();
  remote_write_cancelable_.Cancel();
  rule_cancelable_.Cancel();
}

void Tunnel::Open() {
  NEDEBUG << "Open a new tunnel.";

  ResetTimer();

  open_cancelable_ =
      local_data_flow_->Open([this](utils::Result<void>&& result) {
        if (!result) {
          NEERROR << "Error happened when opening a new tunnel, error code is: "
                  << result.error() << ".";

          ReleaseTunnel();
          return;
        }

        NEDEBUG << "Tunnel successfully opened, matching rule.";

        ResetTimer();
        MatchRule();
      });
}

utils::Runloop* Tunnel::GetRunloop() { return session_->GetRunloop(); }

void Tunnel::MatchRule() {
  rule_cancelable_ = rule_manager_->Match(
      session_,
      [this](utils::Result<std::shared_ptr<rule::RuleInterface>>&& rule) {
        if (!rule) {
          LocalReportError(std::move(rule).error());
          return;
        }

        remote_data_flow_ = (**rule).GetDataFlow(session_);
        ResetTimer();
        ConnectToRemote();
      });
}

void Tunnel::ConnectToRemote() {
  rule_cancelable_ = remote_data_flow_->Connect(
      session_->endpoint()->Dup(), [this](utils::Result<void>&& result) {
        if (!result) {
          LocalReportError(std::move(result).error());
          return;
        }
        ResetTimer();
        FinishLocalNegotiation();
      });
}

void Tunnel::FinishLocalNegotiation() {
  open_cancelable_ =
      local_data_flow_->Continue([this](utils::Result<void>&& result) {
        if (!result) {
          ReleaseTunnel();
          return;
        }
        ResetTimer();
        BeginForward();
      });
}

void Tunnel::BeginForward() {
  ForwardLocal();
  ForwardRemote();
}

void Tunnel::ForwardLocal() {
  CheckTunnelStatus();

  BOOST_ASSERT(local_data_flow_->StateMachine().IsReadable());
  BOOST_ASSERT(remote_data_flow_->StateMachine().IsWritable());

  local_read_cancelable_ =
      local_data_flow_->Read([this](utils::Result<utils::Buffer>&& buffer) {
        ResetTimer();

        if (!buffer) {
          if (utils::CommonErrorCategory::IsEof(buffer.error())) {
            // Close remote write if it is not closed yet.
            if (remote_data_flow_->StateMachine().IsWriteClosable()) {
              remote_write_cancelable_ =
                  remote_data_flow_->CloseWrite([this](utils::Result<void>&&) {
                    ResetTimer();
                    CheckTunnelStatus();
                  });
              return;
            } else {
              CheckTunnelStatus();
            }
            return;
          }

          ReleaseTunnel();
          return;
        }

        remote_write_cancelable_ = remote_data_flow_->Write(
            *std::move(buffer), [this](utils::Result<void>&& result) {
              ResetTimer();

              if (!result) {
                LocalReportError(std::move(result).error());
                return;
              }
              ForwardLocal();
            });
      });
}

void Tunnel::ForwardRemote() {
  CheckTunnelStatus();

  BOOST_ASSERT(local_data_flow_->StateMachine().IsWritable());
  BOOST_ASSERT(remote_data_flow_->StateMachine().IsReadable());

  remote_read_cancelable_ =
      remote_data_flow_->Read([this](utils::Result<utils::Buffer>&& buffer) {
        ResetTimer();

        if (!buffer) {
          if (utils::CommonErrorCategory::IsEof(buffer.error())) {
            if (local_data_flow_->StateMachine().IsWriteClosable()) {
              local_write_cancelable_ =
                  local_data_flow_->CloseWrite([this](utils::Result<void>) {
                    ResetTimer();
                    CheckTunnelStatus();
                  });
            } else {
              CheckTunnelStatus();
            }
            return;
          }

          LocalReportError(std::move(buffer).error());
          return;
        }
        local_write_cancelable_ = local_data_flow_->Write(
            *std::move(buffer), [this](utils::Result<void> result) {
              ResetTimer();

              if (!result) {
                ReleaseTunnel();
                return;
              }
              ForwardRemote();
            });
      });
}

void Tunnel::LocalReportError(utils::Error&&) { ReleaseTunnel(); }

void Tunnel::CheckTunnelStatus() {
  if (local_data_flow_->StateMachine().State() ==
          data_flow::FlowState::Closed &&
      (!remote_data_flow_ || (remote_data_flow_->StateMachine().State() ==
                              data_flow::FlowState::Closed))) {
    ReleaseTunnel();
  }
}

void Tunnel::ReleaseTunnel() { tunnel_manager_->NotifyClosed(this); }

void Tunnel::ResetTimer() { timeout_timer_.Wait(TIMEOUT_INTERVAL); }

Tunnel& TunnelManager::Build(
    std::unique_ptr<data_flow::LocalDataFlowInterface>&& local_data_flow,
    rule::RuleManager* rule_manager) {
  auto tunnel = std::make_unique<transport::Tunnel>(std::move(local_data_flow),
                                                    rule_manager);
  tunnel->tunnel_manager_ = this;

  auto tunnel_ptr = tunnel.get();

  tunnels_[tunnel.get()] = std::move(tunnel);

  NEDEBUG << "Created new tunnel, there are " << tunnels_.size() << " tunnels.";
  return *tunnel_ptr;
}

void TunnelManager::CloseAll() { tunnels_.clear(); }

void TunnelManager::NotifyClosed(Tunnel* tunnel) {
  tunnels_.erase(tunnel);
  NEDEBUG << "Removed one tunnel, there are " << tunnels_.size() << " tunnels.";
}

}  // namespace transport
}  // namespace nekit
