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

#ifndef NEKIT_STREAM_CODER_ACTION_REQUEST
#define NEKIT_STREAM_CODER_ACTION_REQUEST

namespace nekit {
namespace stream_coder {

enum ActionRequest {
  kContinue = 0,   // Continue forwarding data.
  kRemoveSelf,     // The coder is not useful anymore and should be removed.
  kErrorHappened,  // Some error happened, the coder can no longer process data.

  // The following is only used in negotiating phase.
  kWantRead,   // The coder wants to read data to decode.
  kWantWrite,  // The coder wants to write data.
  kReady       // Negotiating finished, ready to forward data.
};
}
}  // namespace nekit

#endif /* NEKIT_STREAM_CODER_ACTION_REQUEST */
