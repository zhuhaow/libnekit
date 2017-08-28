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
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "Tunnel"

namespace nekit {
namespace transport {

using stream_coder::ActionRequest;

Tunnel::Tunnel(std::unique_ptr<ConnectionInterface>&& local_transport,
               std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
                   local_stream_coder,
               rule::RuleManager* rule_manager)
    : rule_manager_{rule_manager},
      local_transport_{std::move(local_transport)},
      local_stream_coder_{std::move(local_stream_coder)},
      incoming_buffer_{
          std::make_unique<utils::Buffer>(NEKIT_TUNNEL_BUFFER_SIZE)},
      outgoing_buffer_{
          std::make_unique<utils::Buffer>(NEKIT_TUNNEL_BUFFER_SIZE)} {}

void Tunnel::Open() {
  ProcessLocalNegotiation(local_stream_coder_->Negotiate());
}

void Tunnel::ProcessLocalNegotiation(ActionRequest request) {
  switch (request) {
    case ActionRequest::WantRead: {
      if (polling_) {
        pending_action_ = PendingAction::NegotiationRead;
        return;
      }

      ResetOutgoingBuffer(local_stream_coder_->DecodeReserve());

      outgoing_cancelable_ = local_transport_->Read(
          std::move(outgoing_buffer_),
          [this](std::unique_ptr<utils::Buffer>&& buffer, std::error_code ec) {
            if (ec) {
              ReleaseTunnel();
              return;
            }

            auto action = local_stream_coder_->Decode(buffer.get());

            ReturnOutgoingBuffer(std::move(buffer));

            ProcessLocalNegotiation(action);
          });
    } break;
    case ActionRequest::ReadyAfterRead: {
      ResetOutgoingBuffer(local_stream_coder_->DecodeReserve() +
                          remote_stream_coder_->EncodeReserve());
      outgoing_buffer_->ShrinkSize();

      switch (local_stream_coder_->Decode(outgoing_buffer_.get())) {
        case ActionRequest::Continue: {
          switch (remote_stream_coder_->Encode(outgoing_buffer_.get())) {
            case ActionRequest::Continue:
              outgoing_cancelable_ = remote_transport_->Write(
                  std::move(outgoing_buffer_),
                  [this](std::unique_ptr<utils::Buffer>&& buffer,
                         std::error_code ec) {
                    (void)buffer;

                    if (ec) {
                      HandleRemoteWriteError(ec);
                      return;
                    }

                    ReturnOutgoingBuffer(std::move(buffer));

                    BeginForward();
                  });
              break;
            default:
              assert(0);
          }
        } break;
        default:
          assert(0);
      }
    } break;
    case ActionRequest::WantWrite: {
      ResetIncomingBuffer(local_stream_coder_->EncodeReserve());
      incoming_buffer_->ShrinkSize();
      auto request = local_stream_coder_->Encode(incoming_buffer_.get());

      incoming_cancelable_ = local_transport_->Write(
          std::move(incoming_buffer_),
          [this, request](std::unique_ptr<utils::Buffer>&& buffer,
                          std::error_code ec) {
            if (ec) {
              ReleaseTunnel();
              return;
            }

            ReturnIncomingBuffer(std::move(buffer));

            ProcessLocalNegotiation(request);
          });
    } break;
    case ActionRequest::ErrorHappened: {
      ReleaseTunnel();
    } break;
    case ActionRequest::Event: {
      session_ = local_stream_coder_->session();

      polling_ = true;
      poll_cancelable_ = local_transport_->PollRead([this](std::error_code ec) {
        polling_ = false;

        if (ec) {
          ReleaseTunnel();
          return;
        }

        NEINFO << "Read polling finished, processing pending action "
               << static_cast<int>(pending_action_) << " for session "
               << session_->domain()->domain();
        switch (pending_action_) {
          case PendingAction::None:
            break;
          case PendingAction::NegotiationRead:
            ProcessLocalNegotiation(ActionRequest::WantRead);
            break;
          case PendingAction::Forward:
            ForwardLocal();
            break;
        }

        pending_action_ = PendingAction::None;
      });
      ProcessSession();
    } break;
    case ActionRequest::Ready:
      BeginForward();
      break;
    case ActionRequest::Continue:
    case ActionRequest::RemoveSelf:
      assert(0);
  }
}

void Tunnel::ProcessRemoteNegotiation(ActionRequest request) {
  switch (request) {
    case ActionRequest::WantRead: {
      ResetIncomingBuffer(remote_stream_coder_->DecodeReserve());

      incoming_cancelable_ = remote_transport_->Read(
          std::move(incoming_buffer_),
          [this](std::unique_ptr<utils::Buffer>&& buffer, std::error_code ec) {
            if (ec) {
              ProcessLocalNegotiation(local_stream_coder_->ReportError(ec));
              return;
            }

            auto action = remote_stream_coder_->Decode(buffer.get());

            ReturnIncomingBuffer(std::move(buffer));

            ProcessRemoteNegotiation(action);
          });
    } break;
    case ActionRequest::WantWrite: {
      ResetOutgoingBuffer(remote_stream_coder_->EncodeReserve());
      outgoing_buffer_->ShrinkSize();
      auto request = remote_stream_coder_->Encode(outgoing_buffer_.get());

      outgoing_cancelable_ = remote_transport_->Write(
          std::move(outgoing_buffer_),
          [this, request](std::unique_ptr<utils::Buffer>&& buffer,
                          std::error_code ec) {
            if (ec) {
              ProcessLocalNegotiation(local_stream_coder_->ReportError(ec));
              return;
            }

            ReturnOutgoingBuffer(std::move(buffer));

            ProcessRemoteNegotiation(request);
          });
    } break;
    case ActionRequest::ErrorHappened: {
      ProcessLocalNegotiation(local_stream_coder_->ReportError(
          remote_stream_coder_->GetLastError()));
    } break;
    case ActionRequest::Ready:
      ProcessLocalNegotiation(local_stream_coder_->Continue());
      break;
    case ActionRequest::Continue:
    case ActionRequest::RemoveSelf:
    case ActionRequest::Event:
    case ActionRequest::ReadyAfterRead:
      assert(0);
  }
}

void Tunnel::BeginForward() {
  ForwardLocal();
  ForwardRemote();
}

void Tunnel::ForwardLocal() {
  if (polling_) {
    pending_action_ = PendingAction::Forward;
    return;
  }

  ResetOutgoingBuffer(local_stream_coder_->DecodeReserve() +
                      remote_stream_coder_->EncodeReserve());

  outgoing_cancelable_ = local_transport_->Read(
      std::move(outgoing_buffer_),
      [this](std::unique_ptr<utils::Buffer>&& buffer,
             std::error_code ec) mutable {
        if (ec) {
          if (ec == ConnectionInterface::ErrorCode::EndOfFile) {
            remote_transport_->CloseWrite();
            CheckTunnelStatus();
            return;
          }

          HandleLocalReadError(ec);
          return;
        }

        switch (local_stream_coder_->Decode(buffer.get())) {
          case ActionRequest::Continue: {
            switch (remote_stream_coder_->Encode(buffer.get())) {
              case ActionRequest::Continue:
                outgoing_cancelable_ = remote_transport_->Write(
                    std::move(buffer),
                    [this](std::unique_ptr<utils::Buffer>&& buffer,
                           std::error_code ec) {
                      (void)buffer;

                      if (ec) {
                        HandleRemoteWriteError(ec);
                        return;
                      }

                      ReturnOutgoingBuffer(std::move(buffer));

                      ForwardLocal();
                    });
                break;
              default:
                assert(0);
            }
          } break;

          case ActionRequest::ErrorHappened:
            HandleLocalReadError(local_stream_coder_->GetLastError());
            break;
          default:
            assert(0);
        }
      });
}

void Tunnel::ForwardRemote() {
  ResetIncomingBuffer(local_stream_coder_->EncodeReserve() +
                      remote_stream_coder_->DecodeReserve());
  incoming_cancelable_ = remote_transport_->Read(
      std::move(incoming_buffer_),
      [this](std::unique_ptr<utils::Buffer>&& buffer,
             std::error_code ec) mutable {
        if (ec) {
          if (ec == ConnectionInterface::ErrorCode::EndOfFile) {
            local_transport_->CloseWrite();
            CheckTunnelStatus();
            return;
          }

          HandleRemoteReadError(ec);
          return;
        }

        switch (remote_stream_coder_->Decode(buffer.get())) {
          case ActionRequest::Continue:
            switch (local_stream_coder_->Encode(buffer.get())) {
              case ActionRequest::Continue:
                incoming_cancelable_ = local_transport_->Write(
                    std::move(buffer),
                    [this](std::unique_ptr<utils::Buffer>&& buffer,
                           std::error_code ec) {
                      (void)buffer;

                      if (ec) {
                        HandleLocalWriteError(ec);
                        return;
                      }

                      ReturnIncomingBuffer(std::move(buffer));

                      ForwardRemote();
                    });
                break;
              default:
                assert(0);
            }
            break;

          case ActionRequest::ErrorHappened:
            HandleRemoteReadError(remote_stream_coder_->GetLastError());
            break;
          default:
            assert(0);
        }
      });
}

void Tunnel::ProcessSession() {
  session_->domain()->set_resolver(rule_manager_->resolver().get());

  rule_cancelable_ = rule_manager_->Match(
      session_,
      [this](std::shared_ptr<rule::RuleInterface> rule, std::error_code ec) {
        if (ec) {
          ProcessLocalNegotiation(local_stream_coder_->ReportError(ec));
          return;
        }

        adapter_ = rule->GetAdapter(session_);
        rule_cancelable_ = adapter_->Open(
            [this](std::unique_ptr<ConnectionInterface>&& conn,
                   std::unique_ptr<stream_coder::StreamCoderInterface>&&
                       stream_coder,
                   std::error_code ec) {
              if (ec) {
                ProcessLocalNegotiation(local_stream_coder_->ReportError(ec));
                return;
              }

              remote_transport_ = std::move(conn);
              remote_stream_coder_ = std::move(stream_coder);
              ProcessRemoteNegotiation(remote_stream_coder_->Negotiate());
            });
      });
}

void Tunnel::HandleLocalReadError(std::error_code ec) {
  (void)ec;
  ReleaseTunnel();
}

void Tunnel::HandleLocalWriteError(std::error_code ec) {
  (void)ec;
  ReleaseTunnel();
}

void Tunnel::HandleRemoteReadError(std::error_code ec) {
  (void)ec;
  ReleaseTunnel();
}

void Tunnel::HandleRemoteWriteError(std::error_code ec) {
  (void)ec;
  ReleaseTunnel();
}

void Tunnel::CheckTunnelStatus() {
  if (local_transport_->IsClosed() &&
      (!remote_transport_ || remote_transport_->IsClosed())) {
    ReleaseTunnel();
  }
}

void Tunnel::ReleaseTunnel() { tunnel_manager_->NotifyClosed(this); }

void Tunnel::ResetIncomingBuffer(const utils::BufferReserveSize& reserve) {
  if (!incoming_buffer_->Reset(reserve)) {
    incoming_buffer_ =
        std::make_unique<utils::Buffer>(reserve, NEKIT_TUNNEL_BUFFER_SIZE);
  };
}

void Tunnel::ReturnIncomingBuffer(std::unique_ptr<utils::Buffer>&& buffer) {
  if (buffer->size() > NEKIT_TUNNEL_MAX_BUFFER_SIZE) {
    incoming_buffer_ =
        std::make_unique<utils::Buffer>(NEKIT_TUNNEL_BUFFER_SIZE);
  } else {
    incoming_buffer_ = std::move(buffer);
  }
}

void Tunnel::ResetOutgoingBuffer(const utils::BufferReserveSize& reserve) {
  if (!outgoing_buffer_->Reset(reserve)) {
    outgoing_buffer_ =
        std::make_unique<utils::Buffer>(reserve, NEKIT_TUNNEL_BUFFER_SIZE);
  };
}

void Tunnel::ReturnOutgoingBuffer(std::unique_ptr<utils::Buffer>&& buffer) {
  if (buffer->size() > NEKIT_TUNNEL_MAX_BUFFER_SIZE) {
    outgoing_buffer_ =
        std::make_unique<utils::Buffer>(NEKIT_TUNNEL_BUFFER_SIZE);
  } else {
    outgoing_buffer_ = std::move(buffer);
  }
}

Tunnel& TunnelManager::Build(
    std::unique_ptr<ConnectionInterface>&& local_transport,
    std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
        local_stream_coder,
    rule::RuleManager* rule_manager) {
  auto tunnel = std::make_unique<transport::Tunnel>(
      std::move(local_transport), std::move(local_stream_coder), rule_manager);
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
