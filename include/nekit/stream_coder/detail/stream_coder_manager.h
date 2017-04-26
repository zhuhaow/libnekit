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

#ifndef NEKIT_STREAM_CODER_DETAIL_STREAM_CODER_MANAGER
#define NEKIT_STREAM_CODER_DETAIL_STREAM_CODER_MANAGER

#include <list>
#include <memory>

#include <boost/core/noncopyable.hpp>

#include <nekit/stream_coder/stream_coder.h>

namespace nekit {
namespace stream_coder {
namespace detail {
class StreamCoderManager final : public boost::noncopyable {
 public:
  typedef std::list<std::unique_ptr<StreamCoder>>::iterator StreamCoderIterator;

  StreamCoderManager();

  void PrependStreamCoder(std::unique_ptr<StreamCoder>&& coder);

  ActionRequest Negotiate();

  BufferReserveSize InputReserve();
  ActionRequest Input(utils::Buffer& buffer);

  BufferReserveSize OutputReserve();
  ActionRequest Output(utils::Buffer& buffer);

  utils::Error GetLatestError() const;

  bool forwarding() const;

 private:
  enum Phase { kInvalid, kNegotiating, kForwarding, kClosed };

  ActionRequest NegotiateNextCoder();
  StreamCoderIterator FindTailIterator();

  ActionRequest InputForNegotiation(utils::Buffer& buffer);
  ActionRequest InputForForward(utils::Buffer& buffer);

  ActionRequest OutputForNegotiation(utils::Buffer& buffer);
  ActionRequest OutputForForward(utils::Buffer& buffer);

  bool VerifyNonEmpty();

  std::list<std::unique_ptr<StreamCoder>> list_;
  StreamCoderIterator active_coder_;
  utils::Error last_error_;
  Phase status_;
};
}  // namespace detail
}  // namespace stream_coder
}  // namespace nekit

#endif /* NEKIT_STREAM_CODER_DETAIL_STREAM_CODER_MANAGER */
