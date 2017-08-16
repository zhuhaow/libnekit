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

#include "nekit/transport/server_adapter.h"

namespace nekit {
namespace transport {
ServerAdapter::ServerAdapter(
    boost::asio::io_service &io, std::shared_ptr<utils::Session> session,
    std::shared_ptr<ConnectorFactoryInterface> connector_factory,
    std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
        stream_coder_factory,
    const boost::asio::ip::address &address, uint16_t port)
    : AdapterInterface{io},
      session_{session},
      connector_factory_{connector_factory},
      stream_coder_factory_{stream_coder_factory},
      address_{address},
      port_{port} {}

ServerAdapter::ServerAdapter(
    boost::asio::io_service &io, std::shared_ptr<utils::Session> session,
    std::shared_ptr<ConnectorFactoryInterface> connector_factory,
    std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
        stream_coder_factory,
    const std::string &domain, uint16_t port,
    utils::ResolverInterface *resolver)
    : AdapterInterface{io},
      session_{session},
      connector_factory_{connector_factory},
      stream_coder_factory_{stream_coder_factory},
      domain_{std::make_shared<utils::Domain>(domain)},
      port_{port} {
  domain_->set_resolver(resolver);
}

const utils::Cancelable &ServerAdapter::Open(EventHandler handler) {
  if (domain_) {
    connector_ = connector_factory_->Build(domain_, port_);
  } else {
    connector_ = connector_factory_->Build(address_, port_);
  }
  cancelable_ = connector_->Connect([
    this, cancelable{life_time_cancelable_pointer()}, handler
  ](std::unique_ptr<ConnectionInterface> && conn, std::error_code ec) {
    if (cancelable->canceled()) {
      return;
    }

    if (ec) {
      handler(nullptr, nullptr, ec);
      return;
    }

    handler(std::move(conn), stream_coder_factory_->Build(session_), ec);
    return;
  });

  return life_time_cancelable();
}

ServerAdapterFactory::ServerAdapterFactory(
    boost::asio::io_service &io,
    std::shared_ptr<ConnectorFactoryInterface> connector_factory,
    std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
        stream_coder_factory,
    boost::asio::ip::address address, uint16_t port)
    : AdapterFactoryInterface{io},
      connector_factory_{connector_factory},
      stream_coder_factory_{stream_coder_factory},
      address_{address},
      port_{port} {}

ServerAdapterFactory::ServerAdapterFactory(
    boost::asio::io_service &io,
    std::shared_ptr<ConnectorFactoryInterface> connector_factory,
    std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
        stream_coder_factory,
    std::string domain, uint16_t port, utils::ResolverInterface *resolver)
    : AdapterFactoryInterface{io},
      connector_factory_{connector_factory},
      stream_coder_factory_{stream_coder_factory},
      resolver_{resolver},
      domain_{domain},
      port_{port} {}

std::unique_ptr<AdapterInterface> ServerAdapterFactory::Build(
    std::shared_ptr<utils::Session> session) {
  if (domain_.empty()) {
    return std::make_unique<ServerAdapter>(io(), session, connector_factory_,
                                           stream_coder_factory_, address_,
                                           port_);
  } else {
    return std::make_unique<ServerAdapter>(io(), session, connector_factory_,
                                           stream_coder_factory_, domain_,
                                           port_, resolver_);
  }
}
}  // namespace transport
}  // namespace nekit
