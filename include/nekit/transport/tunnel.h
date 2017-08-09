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

#include "../rule/rule_manager.h"
#include "../stream_coder/server_stream_coder_interface.h"
#include "../stream_coder/stream_coder_interface.h"
#include "../utils/cancelable.h"
#include "../utils/session.h"
#include "adapter_interface.h"
#include "transport_interface.h"

namespace nekit {
namespace transport {

class TunnelManager;

class Tunnel final : private boost::noncopyable {
 public:
  Tunnel(std::unique_ptr<ConnectionInterface>&& local_transport,
         std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
             local_stream_coder,
         rule::RuleManager* rule_manager);

  void Open();

  friend class TunnelManager;

 private:
  void ProcessLocalNegotiation(stream_coder::ActionRequest action_request);
  void ProcessRemoteNegotiation(stream_coder::ActionRequest action_request);
  void ProcessSession();
  void BeginForward();
  void ForwardLocal();
  void ForwardRemote();

  // Only handling error in forwarding state.
  void HandleLocalReadError(std::error_code ec);
  void HandleLocalWriteError(std::error_code ec);
  void HandleRemoteReadError(std::error_code ec);
  void HandleRemoteWriteError(std::error_code ec);

  void CheckTunnelStatus();
  void ReleaseTunnel();

  void ResetIncomingBuffer(const utils::BufferReserveSize& reserve);
  void ReturnIncomingBuffer(std::unique_ptr<utils::Buffer>&& buffer);
  void ResetOutgoingBuffer(const utils::BufferReserveSize& reserve);
  void ReturnOutgoingBuffer(std::unique_ptr<utils::Buffer>&& buffer);

  std::shared_ptr<utils::Session> session_;
  std::unique_ptr<AdapterInterface> adapter_;

  rule::RuleManager* rule_manager_;
  TunnelManager* tunnel_manager_;

  std::unique_ptr<ConnectionInterface> local_transport_, remote_transport_;
  std::unique_ptr<stream_coder::StreamCoderInterface> remote_stream_coder_;
  std::unique_ptr<stream_coder::ServerStreamCoderInterface> local_stream_coder_;
  std::unique_ptr<utils::Buffer> incoming_buffer_, outgoing_buffer_;

  utils::Cancelable incoming_cancelable_, outgoing_cancelable_;
};

class TunnelManager final : private boost::noncopyable {
 public:
  Tunnel& Build(std::unique_ptr<ConnectionInterface>&& local_transport,
                std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
                    local_stream_coder,
                rule::RuleManager* rule_manager);

  void CloseAll();

  friend class Tunnel;

 private:
  void NotifyClosed(Tunnel* tunnel);

  std::unordered_map<void*, std::unique_ptr<transport::Tunnel>> tunnels_;
};
}  // namespace transport
}  // namespace nekit
