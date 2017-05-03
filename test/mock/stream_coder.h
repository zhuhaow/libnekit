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

#ifndef NEKIT_TEST_STREAM_CODER_MOCK_STREAM_CODER
#define NEKIT_TEST_STREAM_CODER_MOCK_STREAM_CODER

#include <gmock/gmock.h>

#include <nekit/stream_coder/stream_coder_interface.h>

namespace nekit {
namespace stream_coder {

class MockStreamCoder : public StreamCoderInterface {
 public:
  MOCK_METHOD0(Negotiate, ActionRequest());
  MOCK_CONST_METHOD0(InputReserve, BufferReserveSize());
  MOCK_METHOD1(Input, ActionRequest(utils::Buffer& buffer));
  MOCK_CONST_METHOD0(OutputReserve, BufferReserveSize());
  MOCK_METHOD1(Output, ActionRequest(utils::Buffer& buffer));
  MOCK_CONST_METHOD0(GetLatestError, utils::Error());
  MOCK_CONST_METHOD0(forwarding, bool());
};

}  // namespace stream_coder
}  // namespace nekit
#endif /* NEKIT_TEST_STREAM_CODER_MOCK_STREAM_CODER */
