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

namespace nekit {
namespace transport {
Tunnel::Tunnel(std::unique_ptr<TransportInterface>&& local_transport,
               std::unique_ptr<stream_coder::ServerStreamCoderInterface>&&
                   local_stream_coder)
    : local_transport_{std::move(local_transport)},
      local_stream_coder_{std::move(local_stream_coder)} {}

void Tunnel::Open() { ProcessNegotiation(local_stream_coder_->Negotiate()); }

void Tunnel::ProcessNegotiation(stream_coder::ActionRequest request) {
  switch (request) {
    case stream_coder::ActionRequest::WantRead: {
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

            ProcessNegotiation(local_stream_coder_->Decode(buffer.get()));
          });
      return;
    } break;
    case stream_coder::ActionRequest::WantWrite: {
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

            ProcessNegotiation(request);
          });
      return;
    } break;
    case stream_coder::ActionRequest::ErrorHappened: {
      HandleError(local_stream_coder_->GetLastError());
      return;
    } break;
    case stream_coder::ActionRequest::Event: {
      session_ = local_stream_coder_->session();
      ProcessSession();
      return;
    } break;
    case stream_coder::ActionRequest::Ready:
    case stream_coder::ActionRequest::Continue:
    case stream_coder::ActionRequest::RemoveSelf:
      assert(0);
  }
}

void Tunnel::HandleError(std::error_code ec){};
void Tunnel::ProcessSession() {}
}  // namespace transport
}  // namespace nekit
