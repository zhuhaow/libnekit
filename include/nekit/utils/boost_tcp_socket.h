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

#ifndef NEKIT_UTILS_BOOST_TCP_SOCKET
#define NEKIT_UTILS_BOOST_TCP_SOCKET

#include "tcp_socket_interface.h"

namespace nekit {
namespace utils {

class BoostTcpSocket final : public TcpSocketInterface {
 public:
  Pointer CreateSocket(boost::asio::io_service &service);
  Pointer CreateSocket(boost::asio::io_service &service,
                       DelegatePointer delegate);

  void set_delegate(DelegatePointer delegate) override;

  void Connect(const boost::asio::ip::tcp::endpoint endpoint) override;
  Error Bind(const boost::asio::ip::tcp::endpoint endpoint) override;
  Error Shutdown() override;

  void Read(std::shared_ptr<Buffer> buffer) override;
  void Write(const std::shared_ptr<Buffer> buffer) override;

 protected:
  BoostTcpSocket(boost::asio::io_service &service);
  BoostTcpSocket(boost::asio::io_service &service, DelegatePointer delegate);

 private:
  class BoostSocket : public boost::asio::ip::tcp::socket {
    BoostSocket(boost::asio::io_service &service)
        : boost::asio::ip::tcp::socket(service) {}
    friend class BoostTcpSocket;
  };

  void HandleError(const boost::system::error_code &ec);

  Error ConvertBoostError(const boost::system::error_code &ec);

  void CheckClose();

  BoostSocket socket_;
  DelegatePointer delegate_;

  bool connected_;
  bool eof_;
  bool shutdown_;
  bool closed_;
};

}  // namespace utils
}  // namespace nekit

#endif /* NEKIT_UTILS_BOOST_TCP_SOCKET */
