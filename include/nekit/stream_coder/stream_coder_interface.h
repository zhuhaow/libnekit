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

#ifndef NEKIT_STREAM_CODER_STREAM_CODER_INTERFACE
#define NEKIT_STREAM_CODER_STREAM_CODER_INTERFACE

#include <boost/noncopyable.hpp>

#include <nekit/stream_coder/action_request.h>
#include <nekit/stream_coder/buffer_reserve_size.h>
#include <nekit/stream_coder/error.h>
#include <nekit/utils/buffer.h>

namespace nekit {
namespace stream_coder {

class StreamCoderInterface : boost::noncopyable {
 public:
  virtual ~StreamCoderInterface() {}

  virtual ActionRequest Negotiate() = 0;

  virtual BufferReserveSize InputReserve() = 0;
  virtual ActionRequest Input(utils::Buffer& buffer) = 0;

  virtual BufferReserveSize OutputReserve() = 0;
  virtual ActionRequest Output(utils::Buffer& buffer) = 0;

  virtual utils::Error GetLatestError() const = 0;

  virtual bool forwarding() const = 0;
};

}  // namespace stream_coder
}  // namespace nekit

#endif /* NEKIT_STREAM_CODER_STREAM_CODER_INTERFACE */
