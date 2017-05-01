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

#ifndef NEKIT_STREAM_CODER_STREAM_CODER_PIPE
#define NEKIT_STREAM_CODER_STREAM_CODER_PIPE

#include <memory>

#include <nekit/stream_coder/stream_coder_interface.h>

namespace nekit {
namespace stream_coder {

class StreamCoderPipe final : public StreamCoderInterface {
public:
  void AppendStreamCoder(std::unique_ptr<StreamCoderInterface>&& stream_coder);

  ActionRequest Negotiate();

  BufferReserveSize InputReserve();
  ActionRequest Input(utils::Buffer& buffer);

  BufferReserveSize OutputReserve();
  ActionRequest Output(utils::Buffer& buffer);

  utils::Error GetLatestError() const;

  bool forwarding() const;

 private:
  class StreamCoderPipeImplementation;
  std::unique_ptr<StreamCoderPipeImplementation> impl_;
};

}  // namespace stream_coder
}  // namespace nekit

#endif /* NEKIT_STREAM_CODER_STREAM_CODER_PIPE */
