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

#include "nekit/transport/socks5_listener.h"

namespace nekit {
namespace transport {
Socks5Listener::Socks5Listener(
    boost::asio::io_service& io,
    stream_coder::Socks5ServerStreamCoderFactory&& factory)
    : listener_{io}, stream_coder_factory_{std::move(factory)} {}

std::error_code Socks5Listener::Bind(std::string ip, uint16_t port) {
  return listener_.Bind(ip, port);
}

std::error_code Socks5Listener::Bind(boost::asio::ip::address ip,
                                     uint16_t port) {
  return listener_.Bind(ip, port);
}

void Socks5Listener::Accept(EventHandler&& handler) {
  listener_.Accept([ this, handler{std::move(handler)} ](
      std::unique_ptr<ConnectionInterface> && conn, std::error_code ec) {
    if (ec) {
      handler(nullptr, nullptr, ec);
      return;
    }

    handler(std::move(conn), stream_coder_factory_.Build(), ec);
    return;
  });
}

}  // namespace transport
}  // namespace nekit
