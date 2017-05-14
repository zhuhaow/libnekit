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

#ifndef NEKIT_UTILS_TCP_SOCKET_INTERFACE
#define NEKIT_UTILS_TCP_SOCKET_INTERFACE

#include <memory>

#include <boost/asio.hpp>

#include <nekit/utils/buffer.h>
#include <nekit/utils/error.h>

namespace nekit {
namespace utils {

class TcpSocketDelegateInterface;

// TcpSocketInterface defines the interface for TCP streams. It is expected to
// be used with and only with std::shared_ptr. Any concrete subclass should not
// provide public constructor but a helper method returning a std::shared_ptr.
class TcpSocketInterface
    : virtual public std::enable_shared_from_this<TcpSocketInterface> {
 public:
  enum ErrorCode { kNoError = 0, kUnknownError };

  class ErrorCategory final : public std::error_category {
    const char *name() const BOOST_NOEXCEPT override;
    std::string message(int error_code) const override;
  };

  const static std::error_category &error_category();

  typedef std::shared_ptr<TcpSocketInterface> Pointer;
  typedef std::shared_ptr<TcpSocketDelegateInterface> DelegatePointer;

  virtual ~TcpSocketInterface() {}

  virtual void set_delegate(DelegatePointer delegate) = 0;

  // Connect to remote asynchronously.
  void Connect(const std::string &ip_address, unsigned short port) {
    Connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address::from_string(ip_address), port));
  }
  virtual void Connect(const boost::asio::ip::tcp::endpoint endpoint) = 0;

  // Bind socket to specific endpoint.
  Error Bind(const std::string &ip_address, unsigned short port) {
    return Bind(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address::from_string(ip_address), port));
  }
  virtual Error Bind(const boost::asio::ip::tcp::endpoint endpoint) = 0;

  // Shutdown the socket (send FIN).
  // Theoretically there are three types of shutdown: send/receive/both.
  // However, it is rarely needed to shutdown receive in real application, so
  // only shutdown write is provided.
  virtual Error Shutdown() = 0;

  // Read data into buffer.
  virtual void Read(std::shared_ptr<Buffer> buffer) = 0;

  // Send data in buffer to remote.
  virtual void Write(const std::shared_ptr<Buffer> buffer) = 0;
};

class TcpSocketDelegateInterface {
 public:
  virtual ~TcpSocketDelegateInterface() {}

  virtual void DidConnect(typename TcpSocketInterface::Pointer socket) = 0;

  virtual void DidError(const Error &error,
                        typename TcpSocketInterface::Pointer socket) = 0;
  virtual void DidEOF(typename TcpSocketInterface::Pointer socket) = 0;
  virtual void DidClose(typename TcpSocketInterface::Pointer socket) = 0;

  virtual void DidRead(std::shared_ptr<Buffer> buffer,
                       typename TcpSocketInterface::Pointer socket) = 0;
  virtual void DidWrite(const std::shared_ptr<Buffer> buffer,
                        typename TcpSocketInterface::Pointer socket) = 0;
};

}  // namespace utils
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::utils::TcpSocketInterface::ErrorCode>
    : public std::true_type {};

error_code make_error_code(nekit::utils::TcpSocketInterface::ErrorCode errc);
}  // namespace std
#endif /* NEKIT_UTILS_TCP_SOCKET_INTERFACE */
