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

#include <boost/noncopyable.hpp>

#include "nekit/stream_coder/server_stream_coder_interface.h"
#include "nekit/stream_coder/stream_coder_interface.h"
#include "nekit/transport/adapter_interface.h"
#include "nekit/transport/transport_interface.h"
#include "nekit/utils/session.h"

namespace nekit {
namespace transport {
class Tunnel final : private boost::noncopyable {
 public:
  Tunnel(std::unique_ptr<TransportInterface>&& local_transport,
         std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
             local_stream_coder);

  void Open();

 private:
  void ProcessLocalNegotiation(stream_coder::ActionRequest action_request);
  void ProcessRemoteNegotiation(stream_coder::ActionRequest action_request);
  void BeginForward();
  void ForwardLocal();
  void ForwardRemote();
  void HandleError(std::error_code ec);
  void ProcessSession();

  std::shared_ptr<utils::Session> session_;
  std::unique_ptr<AdapterInterface> adapter_;
  std::unique_ptr<TransportInterface> local_transport_, remote_transport_;
  std::unique_ptr<stream_coder::StreamCoderInterface> remote_stream_coder_;
  std::unique_ptr<stream_coder::ServerStreamCoderInterface> local_stream_coder_;
};
}  // namespace transport
}  // namespace nekit
