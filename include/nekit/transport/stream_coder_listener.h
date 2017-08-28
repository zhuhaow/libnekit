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

#include "../stream_coder/server_stream_coder_interface.h"
#include "../utils/log.h"
#include "server_listener_interface.h"
#include "tcp_listener.h"

#undef NECHANNEL
#define NECHANNEL "Stream Listener"

namespace nekit {
namespace transport {
template <typename StreamCoderFactory>
class StreamCoderListener : public ServerListenerInterface {
 public:
  StreamCoderListener(boost::asio::io_service& io,
                      std::unique_ptr<StreamCoderFactory> factory)
      : ServerListenerInterface{io},
        listener_{io},
        stream_coder_factory_{std::move(factory)} {}

  std::error_code Bind(std::string ip, uint16_t port) {
    return listener_.Bind(ip, port);
  }

  std::error_code Bind(boost::asio::ip::address ip, uint16_t port) {
    return listener_.Bind(ip, port);
  }

  void Accept(EventHandler handler) override {
    NEDEBUG << "Start accepting new socket.";

    listener_.Accept(
        [this, handler](std::unique_ptr<ConnectionInterface>&& conn,
                        std::error_code ec) mutable {
          if (ec) {
            NEERROR << "Failed to accept socket due to " << ec << ".";
            handler(nullptr, nullptr, ec);
            return;
          }

          NEINFO << "Accepted new socket.";

          handler(std::move(conn), stream_coder_factory_->Build(), ec);
          Accept(handler);
          return;
        });
  }

  void Close() override { listener_.Close(); }

 private:
  TcpListener listener_;
  std::unique_ptr<StreamCoderFactory> stream_coder_factory_;
};
}  // namespace transport
}  // namespace nekit
