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

#include "../stream_coder/stream_coder_interface.h"
#include "../utils/resolver_interface.h"
#include "adapter_interface.h"
#include "connector_interface.h"

namespace nekit {
namespace transport {

class ServerAdapter;

class ServerAdapterFactory : public AdapterFactoryInterface {
 public:
  ServerAdapterFactory(
      boost::asio::io_service &io,
      std::shared_ptr<ConnectorFactoryInterface> connector_factory,
      std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
          stream_coder_factory,
      boost::asio::ip::address address, uint16_t port);

  ServerAdapterFactory(
      boost::asio::io_service &io,
      std::shared_ptr<ConnectorFactoryInterface> connector_factory,
      std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
          stream_coder_factory,
      std::string domain, uint16_t port, utils::ResolverInterface *resolver);

  std::unique_ptr<AdapterInterface> Build(
      std::shared_ptr<utils::Session> session) override;

 private:
  std::shared_ptr<ConnectorFactoryInterface> connector_factory_;
  std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
      stream_coder_factory_;
  utils::ResolverInterface *resolver_;

  boost::asio::ip::address address_;
  std::string domain_;
  uint16_t port_;
};

class ServerAdapter : public AdapterInterface, private utils::LifeTime {
 public:
  using Factory = ServerAdapterFactory;

  ServerAdapter(boost::asio::io_service &io,
                std::shared_ptr<utils::Session> session,
                std::shared_ptr<ConnectorFactoryInterface> connector_factory,
                std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
                    stream_coder_factory,
                const boost::asio::ip::address &address, uint16_t port);

  ServerAdapter(boost::asio::io_service &io,
                std::shared_ptr<utils::Session> session,
                std::shared_ptr<ConnectorFactoryInterface> connector_factory,
                std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
                    stream_coder_factory,
                const std::string &domain, uint16_t port,
                utils::ResolverInterface *resolver);

  const utils::Cancelable &Open(EventHandler handler) override
      __attribute__((warn_unused_result));

 private:
  std::shared_ptr<utils::Session> session_;
  std::shared_ptr<ConnectorFactoryInterface> connector_factory_;
  std::shared_ptr<stream_coder::StreamCoderFactoryInterface>
      stream_coder_factory_;

  std::unique_ptr<ConnectorInterface> connector_;

  boost::asio::ip::address address_;
  std::shared_ptr<utils::Domain> domain_;
  uint16_t port_;

  utils::Cancelable cancelable_;
};

}  // namespace transport
}  // namespace nekit
