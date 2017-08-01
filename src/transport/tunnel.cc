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
#include "nekit/utils/runtime.h"

namespace nekit {
namespace transport {

using stream_coder::ActionRequest;

Tunnel::Tunnel(std::unique_ptr<TransportInterface>&& local_transport,
               std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
                   local_stream_coder)
    : local_transport_{std::move(local_transport)},
      local_stream_coder_{std::move(local_stream_coder)} {}

void Tunnel::Open() {
  ProcessLocalNegotiation(local_stream_coder_->Negotiate());
}

void Tunnel::ProcessLocalNegotiation(ActionRequest request) {
  switch (request) {
    case ActionRequest::WantRead: {
      auto buffer = std::make_unique<utils::Buffer>(
          local_stream_coder_->DecodeReserve(),
          NEKIT_TUNNEL_NEGOTIATION_CONTENT_SIZE);
      local_transport_->Read(
          std::move(buffer),
          [this](std::unique_ptr<utils::Buffer>&& buffer, std::error_code ec) {
            if (ec) {
              HandleError(ec);
              return;
            }

            ProcessLocalNegotiation(local_stream_coder_->Decode(buffer.get()));
          });
      return;
    } break;
    case ActionRequest::WantWrite: {
      auto buffer =
          std::make_unique<utils::Buffer>(local_stream_coder_->EncodeReserve());
      auto request = local_stream_coder_->Encode(buffer.get());
      local_transport_->Write(
          std::move(buffer),
          [this, request](std::unique_ptr<utils::Buffer>&& buffer,
                          std::error_code ec) {
            (void)buffer;

            if (ec) {
              HandleError(ec);
              return;
            }

            ProcessLocalNegotiation(request);
          });
      return;
    } break;
    case ActionRequest::ErrorHappened: {
      HandleError(local_stream_coder_->GetLastError());
      return;
    } break;
    case ActionRequest::Event: {
      session_ = local_stream_coder_->session();
      ProcessSession();
      return;
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
      auto buffer = std::make_unique<utils::Buffer>(
          remote_stream_coder_->DecodeReserve(),
          NEKIT_TUNNEL_NEGOTIATION_CONTENT_SIZE);
      remote_transport_->Read(
          std::move(buffer),
          [this](std::unique_ptr<utils::Buffer>&& buffer, std::error_code ec) {
            if (ec) {
              HandleError(ec);
              return;
            }

            ProcessRemoteNegotiation(
                remote_stream_coder_->Decode(buffer.get()));
          });
      return;
    } break;
    case ActionRequest::WantWrite: {
      auto buffer = std::make_unique<utils::Buffer>(
          remote_stream_coder_->EncodeReserve());
      auto request = remote_stream_coder_->Encode(buffer.get());
      remote_transport_->Write(
          std::move(buffer),
          [this, request](std::unique_ptr<utils::Buffer>&& buffer,
                          std::error_code ec) {
            (void)buffer;

            if (ec) {
              HandleError(ec);
              return;
            }

            ProcessRemoteNegotiation(request);
          });
      return;
    } break;
    case ActionRequest::ErrorHappened: {
      HandleError(remote_stream_coder_->GetLastError());
      return;
    } break;
    case ActionRequest::Ready:
      ProcessLocalNegotiation(local_stream_coder_->Continue());
      break;
    case ActionRequest::Continue:
    case ActionRequest::RemoveSelf:
    case ActionRequest::Event:
      assert(0);
  }
}

void Tunnel::BeginForward() {
  ForwardLocal();
  ForwardRemote();
}

void Tunnel::ForwardLocal() {
  auto buffer =
      std::make_unique<utils::Buffer>(local_stream_coder_->DecodeReserve() +
                                          remote_stream_coder_->EncodeReserve(),
                                      NEKIT_TUNNEL_FORWARD_CONTENT_SIZE);
  local_transport_->Read(
      std::move(buffer), [this](std::unique_ptr<utils::Buffer>&& buffer,
                                std::error_code ec) mutable {
        if (ec) {
          HandleError(ec);
          return;
        }

        switch (local_stream_coder_->Decode(buffer.get())) {
          case ActionRequest::Continue: {
              switch (remote_stream_coder_->Encode(buffer.get())) {
                  case ActionRequest::Continue:
                      remote_transport_->Write(
                              std::move(buffer),
                              [this](std::unique_ptr<utils::Buffer> &&buffer,
                                     std::error_code ec) {
                                  (void) buffer;
                                  if (ec) {
                                      HandleError(ec);
                                      return;
                                  }

                                  ForwardLocal();
                              });
                      break;
                  case ActionRequest::ErrorHappened:
                      HandleError(remote_stream_coder_->GetLastError());
                      break;
                  default:
                      assert(0);
              }
          }
            break;

          case ActionRequest::ErrorHappened:
            HandleError(local_stream_coder_->GetLastError());
            break;
          default:
            assert(0);
        }
      });
}

void Tunnel::ForwardRemote() {
  auto buffer =
      std::make_unique<utils::Buffer>(local_stream_coder_->EncodeReserve() +
                                          remote_stream_coder_->DecodeReserve(),
                                      NEKIT_TUNNEL_FORWARD_CONTENT_SIZE);
  remote_transport_->Read(
      std::move(buffer), [this](std::unique_ptr<utils::Buffer>&& buffer,
                                std::error_code ec) mutable {
        if (ec) {
          HandleError(ec);
          return;
        }

        switch (remote_stream_coder_->Decode(buffer.get())) {
          case ActionRequest::Continue:

            switch (local_stream_coder_->Encode(buffer.get())) {
              case ActionRequest::Continue:
                local_transport_->Write(
                    std::move(buffer),
                    [this](std::unique_ptr<utils::Buffer>&& buffer,
                           std::error_code ec) {
                      (void)buffer;
                      if (ec) {
                        HandleError(ec);
                        return;
                      }

                      ForwardRemote();
                    });
                break;
              case ActionRequest::ErrorHappened:
                HandleError(local_stream_coder_->GetLastError());
                break;
              default:
                assert(0);
            }
            break;

          case ActionRequest::ErrorHappened:
            HandleError(remote_stream_coder_->GetLastError());
            break;
          default:
            assert(0);
        }
      });
}

void Tunnel::HandleError(std::error_code ec){};

void Tunnel::ProcessSession() {
  utils::Runtime::CurrentRuntime().RuleSet()->Match(
      session_,
      [this](std::shared_ptr<rule::RuleInterface> rule, std::error_code ec) {
        if (ec) {
          HandleError(ec);
          return;
        }

        adapter_ = rule->GetAdapter(session_);
        adapter_->Open(
            [this](std::unique_ptr<TransportInterface>&& conn,
                   std::unique_ptr<stream_coder::StreamCoderInterface>&&
                       stream_coder,
                   std::error_code ec) {
              if (ec) {
                HandleError(ec);
                return;
              }

              remote_transport_ = std::move(conn);
              remote_stream_coder_ = std::move(stream_coder);
              ProcessRemoteNegotiation(remote_stream_coder_->Negotiate());
            });
      });
}
}  // namespace transport
}  // namespace nekit
