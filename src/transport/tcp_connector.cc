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

#include "nekit/transport/tcp_connector.h"

#include <boost/assert.hpp>

#include "nekit/transport/tcp_socket.h"
#include "nekit/utils/boost_error.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "TCP Connector"

namespace nekit {
namespace transport {
TcpConnector::TcpConnector(
    utils::Runloop* runloop,
    std::shared_ptr<const std::vector<boost::asio::ip::address>> addresses,
    uint16_t port)
    : socket_{*runloop->BoostIoContext()},
      addresses_{addresses},
      port_{port},
      runloop_{runloop} {
  BOOST_ASSERT(!addresses_->empty());
}

TcpConnector::TcpConnector(utils::Runloop* runloop,
                           const boost::asio::ip::address& address,
                           uint16_t port)
    : socket_{*runloop->BoostIoContext()},
      address_{address},
      port_{port},
      runloop_{runloop} {}

TcpConnector::TcpConnector(utils::Runloop* runloop,
                           std::shared_ptr<utils::Endpoint> endpoint)
    : socket_{*runloop->BoostIoContext()},
      endpoint_{endpoint},
      port_{endpoint->port()},
      runloop_{runloop} {}

TcpConnector::~TcpConnector() { cancelable_.Cancel(); }

utils::Cancelable TcpConnector::Connect(EventHandler handler) {
  BOOST_ASSERT(!connecting_);

  NEDEBUG << "Begin connecting to remote.";

  if (endpoint_) {
    if (endpoint_->IsAddressAvailable()) {
      switch (endpoint_->type()) {
        case utils::Endpoint::Type::Address:
          address_ = endpoint_->address();
          break;
        case utils::Endpoint::Type::Domain:
          addresses_ = endpoint_->resolved_addresses();
          break;
      }

      NEDEBUG << "Addresses are available, connect directly.";
      DoConnect(handler);
      // Note connector is disposable.
      return cancelable_;
    } else {
      if (!endpoint_->IsResolvable()) {
        NEERROR << "Can not connect since resolve is failed due to "
                << endpoint_->ResolveError() << ".";

        boost::asio::post(
            socket_.get_executor(), [this, handler, cancelable{cancelable_}]() {
              if (cancelable.canceled()) {
                return;
              }

              handler(utils::MakeErrorResult(endpoint_->ResolveError().Dup()));
            });
        return cancelable_;
      } else {
        (void)endpoint_->Resolve([this, handler, cancelable{cancelable_}](
                                     utils::Result<void>&& result) mutable {
          if (cancelable.canceled()) {
            return;
          }

          if (!result) {
            NEERROR << "Can not connect since resolve is failed due to "
                    << result.error() << ".";
            handler(utils::MakeErrorResult(std::move(result).error()));
            return;
          }

          NEDEBUG << "Domain resolved, connect now.";
          addresses_ = endpoint_->resolved_addresses();
          DoConnect(handler);
        });
        return cancelable_;
      }
    }
  }

  NEDEBUG << "Connect request made by addresses, connect directly.";

  DoConnect(handler);

  return cancelable_;
}

void TcpConnector::Bind(std::shared_ptr<utils::DeviceInterface> device) {
  device_ = device;
}

void TcpConnector::DoConnect(EventHandler handler) {
  connecting_ = true;

  if (cancelable_.canceled()) {
    connecting_ = false;
    return;
  }

  if ((!addresses_ && current_ind_) ||
      (addresses_ && current_ind_ >= addresses_->size())) {
    NEERROR << "Fail to connect to all addresses, the last known error is "
            << last_error_ << ".";
    handler(utils::MakeErrorResult(
        utils::BoostErrorCategory::FromBoostError(last_error_)));
    return;
  }

  boost::system::error_code ec;
  socket_.close(ec);
  BOOST_ASSERT(!ec);

  const boost::asio::ip::address* address;
  if (addresses_) {
    address = &addresses_->at(current_ind_);
  } else {
    address = &address_;
  }

  socket_.async_connect(
      boost::asio::ip::tcp::endpoint(*address, port_),
      [this, handler,
       cancelable{cancelable_}](const boost::system::error_code& ec) mutable {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          NEDEBUG << "Connect failed due to " << ec << ", trying next address.";

          if (ec.value() == boost::asio::error::operation_aborted) {
            return;
          }

          last_error_ = ec;
          current_ind_++;
          DoConnect(handler);
          return;
        }

        NEINFO << "Successfully connected to remote.";
        connecting_ = false;
        handler(std::move(socket_));
        return;
      });
}

utils::Runloop* TcpConnector::GetRunloop() { return runloop_; }

}  // namespace transport
}  // namespace nekit
