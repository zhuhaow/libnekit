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

#include <functional>
#include <system_error>
#include <vector>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "../data_flow/local_data_flow_interface.h"
#include "../data_flow/remote_data_flow_interface.h"
#include "tcp_connector.h"
#include "tcp_listener.h"

namespace nekit {
namespace transport {

class TcpSocket final : public data_flow::LocalDataFlowInterface,
                        public data_flow::RemoteDataFlowInterface,
                        private utils::LifeTime {
 public:
  explicit TcpSocket(std::shared_ptr<utils::Session> session);

  const utils::Cancelable& Read(std::unique_ptr<utils::Buffer>&&,
                                DataEventHandler) override
      __attribute__((warn_unused_result));
  const utils::Cancelable& Write(std::unique_ptr<utils::Buffer>&&,
                                 EventHandler) override
      __attribute__((warn_unused_result));

  const utils::Cancelable& CloseWrite(EventHandler) override
      __attribute__((warn_unused_result));

  bool IsReadClosed() const override;
  bool IsWriteClosed() const override;
  bool IsClosed() const override;

  bool IsReading() const override;
  bool IsWriting() const override;

  bool IsIdle() const override;

  bool IsReady() const override;

  data_flow::DataFlowInterface* NextHop() const override;

  std::shared_ptr<utils::Endpoint> ConnectingTo() override;

  data_flow::DataType FlowDataType() const override;

  std::shared_ptr<utils::Session> session() const override;

  boost::asio::io_context* io() override;

  const utils::Cancelable& Open(EventHandler) override
      __attribute__((warn_unused_result));

  const utils::Cancelable& Continue(EventHandler) override
      __attribute__((warn_unused_result));

  const utils::Cancelable& ReportError(std::error_code, EventHandler) override
      __attribute__((warn_unused_result));

  LocalDataFlowInterface* NextLocalHop() const override;

  const utils::Cancelable& Connect(EventHandler) override
      __attribute__((warn_unused_result));

  RemoteDataFlowInterface* NextRemoteHop() const override;

  friend class TcpListener;

 private:
  explicit TcpSocket(boost::asio::ip::tcp::socket&& socket,
                     std::shared_ptr<utils::Session> session);

  std::error_code ConvertBoostError(const boost::system::error_code&) const;

  boost::asio::ip::tcp::socket socket_;
  std::unique_ptr<TcpConnector> connector_;
  std::shared_ptr<utils::Session> session_;
  std::shared_ptr<utils::Endpoint> connect_to_;
  std::unique_ptr<std::vector<boost::asio::const_buffer>> write_buffer_;
  std::unique_ptr<std::vector<boost::asio::mutable_buffer>> read_buffer_;
  bool read_closed_{false}, write_closed_{false}, errored_{false};
  bool reading_{false}, writing_{false}, processing_{false};
  bool ready_{false};
  utils::Cancelable read_cancelable_, write_cancelable_, report_cancelable_,
      connect_cancelable_;
};
}  // namespace transport
}  // namespace nekit
