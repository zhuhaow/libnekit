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
#include "nekit/transport/error_code.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "Tunnel"

#define TIMEOUT_INTERVAL 300

namespace nekit {
namespace transport {

Tunnel::Tunnel(
    std::unique_ptr<data_flow::LocalDataFlowInterface>&& local_data_flow,
    rule::RuleManager* rule_manager)
    : session_{local_data_flow->Session()},
      rule_manager_{rule_manager},
      local_data_flow_{std::move(local_data_flow)},
      timeout_timer_{session_->io(), [this]() { ReleaseTunnel(); }} {}

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

  open_cancelable_ = local_data_flow_->Open([this](std::error_code ec) {
    if (ec) {
      NEERROR << "Error happened when opening a new tunnel, error code is: "
              << ec << ".";

      ReleaseTunnel();
      return;
    }

    NEDEBUG << "Tunnel successfully opened, matching rule.";

    ResetTimer();
    MatchRule();
  });
}

void Tunnel::MatchRule() {
  rule_cancelable_ = rule_manager_->Match(
      session_,
      [this](std::shared_ptr<rule::RuleInterface> rule, std::error_code ec) {
        if (ec) {
          LocalReportError(ec);
          return;
        }

        remote_data_flow_ = rule->GetDataFlow(session_);
        ResetTimer();
        ConnectToRemote();
      });
}

void Tunnel::ConnectToRemote() {
  rule_cancelable_ = remote_data_flow_->Connect([this](std::error_code ec) {
    if (ec) {
      LocalReportError(ec);
      return;
    }

    ResetTimer();
    FinishLocalNegotiation();
  });
}

void Tunnel::FinishLocalNegotiation() {
  open_cancelable_ = local_data_flow_->Continue([this](std::error_code ec) {
    if (ec) {
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

  BOOST_ASSERT(!local_data_flow_->IsReading());
  BOOST_ASSERT(!remote_data_flow_->IsWriting());
  BOOST_ASSERT(local_data_flow_->State() != data_flow::State::Closing ||
               !local_data_flow_->IsReadClosed());
  BOOST_ASSERT(remote_data_flow_->State() != data_flow::State::Closing ||
               !remote_data_flow_->IsWriteClosed());

  local_read_cancelable_ = local_data_flow_->Read(
      CreateBuffer(), [this](utils::Buffer&& buffer, std::error_code ec) {
        ResetTimer();

        if (ec) {
          if (ec == nekit::transport::ErrorCode::EndOfFile) {
            // Close remote write if it is not closed yet.
            if (NE_DATA_FLOW_WRITE_CLOSABLE(remote_data_flow_)) {
              remote_write_cancelable_ =
                  remote_data_flow_->CloseWrite([this](std::error_code ec) {
                    (void)ec;
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

        if ((remote_data_flow_->State() == data_flow::State::Closing &&
             remote_data_flow_->IsWriteClosed()) ||
            remote_data_flow_->State() == data_flow::State::Closed) {
          // This can't happen! Since we don't close the write yet, it
          // must be closed due to error. Then `ReportError` should be called
          // and the read callback should have been canceled.
          BOOST_ASSERT(false);
          ReleaseTunnel();
          return;
        }

        remote_write_cancelable_ = remote_data_flow_->Write(
            std::move(buffer), [this](std::error_code ec) {
              ResetTimer();
              if (ec) {
                LocalReportError(ec);
                return;
              }

              ForwardLocal();
            });
      });
}

void Tunnel::ForwardRemote() {
  CheckTunnelStatus();

  BOOST_ASSERT(!remote_data_flow_->IsReading());
  BOOST_ASSERT(!local_data_flow_->IsWriting());
  BOOST_ASSERT(remote_data_flow_->State() != data_flow::State::Closing ||
               !remote_data_flow_->IsReadClosed());
  BOOST_ASSERT(local_data_flow_->State() != data_flow::State::Closing ||
               !local_data_flow_->IsWriteClosed());

  remote_read_cancelable_ = remote_data_flow_->Read(
      CreateBuffer(), [this](utils::Buffer&& buffer, std::error_code ec) {
        ResetTimer();
        if (ec) {
          if (ec == nekit::transport::ErrorCode::EndOfFile) {
            if (NE_DATA_FLOW_WRITE_CLOSABLE(local_data_flow_)) {
              local_write_cancelable_ =
                  local_data_flow_->CloseWrite([this](std::error_code ec) {
                    (void)ec;
                    ResetTimer();
                    CheckTunnelStatus();
                  });
            } else {
              CheckTunnelStatus();
            }
            return;
          }

          LocalReportError(ec);
          return;
        }

        if (local_data_flow_->IsWriteClosed()) {
          BOOST_ASSERT(false);
          ReleaseTunnel();
          return;
        }

        local_write_cancelable_ = local_data_flow_->Write(
            std::move(buffer), [this](std::error_code ec) {
              ResetTimer();
              if (ec) {
                ReleaseTunnel();
                return;
              }

              ForwardRemote();
            });
      });
}

void Tunnel::LocalReportError(std::error_code ec) {
  local_write_cancelable_ =
      local_data_flow_->ReportError(ec, [this](std::error_code ec) {
        (void)ec;
        ReleaseTunnel();
        return;
      });
}

void Tunnel::CheckTunnelStatus() {
  if (local_data_flow_->State() == data_flow::State::Closed &&
      (!remote_data_flow_ ||
       (remote_data_flow_->State() == data_flow::State::Closed))) {
    ReleaseTunnel();
  }
}

void Tunnel::ReleaseTunnel() { tunnel_manager_->NotifyClosed(this); }

void Tunnel::ResetTimer() { timeout_timer_.Wait(TIMEOUT_INTERVAL); }

utils::Buffer Tunnel::CreateBuffer() {
  return utils::Buffer(NEKIT_TUNNEL_BUFFER_SIZE);
}

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
