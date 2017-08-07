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

#include "../stream_coder/socks5_server_stream_coder.h"
#include "server_listener_interface.h"
#include "tcp_listener.h"

namespace nekit {
namespace transport {
class Socks5Listener : public ServerListenerInterface {
 public:
  Socks5Listener(boost::asio::io_service& io,
                 stream_coder::Socks5ServerStreamCoderFactory&& factory);

  std::error_code Bind(std::string ip, uint16_t port);
  std::error_code Bind(boost::asio::ip::address ip, uint16_t port);

  void Accept(EventHandler handler) override;

  void Close() override;

 private:
  TcpListener listener_;
  stream_coder::Socks5ServerStreamCoderFactory stream_coder_factory_;
};
}  // namespace transport
}  // namespace nekit
