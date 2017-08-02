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

#include "nekit/transport/direct_adapter.h"

namespace nekit {
namespace transport {
DirectAdapter::DirectAdapter(
    std::shared_ptr<utils::Session> session,
    std::shared_ptr<ConnectorFactoryInterface> connector_factory)
    : session_{session}, connector_factory_{connector_factory} {}

void DirectAdapter::Open(EventHandler handler) {
  handler_ = handler;

  if (session_->type() == utils::Session::Type::Address) {
    connector_ =
        connector_factory_->Build(session_->address(), session_->port());
  } else {
    connector_ =
        connector_factory_->Build(session_->domain(), session_->port());
  }

  DoConnect();
}

void DirectAdapter::DoConnect() {
  connector_->Connect(
      [this](std::unique_ptr<ConnectionInterface>&& conn, std::error_code ec) {
        if (ec) {
          handler_(nullptr, nullptr, ec);
          return;
        }

        handler_(std::move(conn), stream_coder_factory_.Build(session_), ec);
        return;
      });
}

DirectAdapterFactory::DirectAdapterFactory(
    std::shared_ptr<ConnectorFactoryInterface> connector_factory)
    : connector_factory_{std::move(connector_factory)} {}

std::unique_ptr<AdapterInterface> DirectAdapterFactory::Build(
    std::shared_ptr<utils::Session> session) {
  return std::make_unique<DirectAdapter>(session, connector_factory_);
}
}  // namespace transport
}  // namespace nekit
