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

#include "nekit/stream_coder/direct_stream_coder.h"
#include "nekit/utils/no_error.h"

namespace nekit {
namespace stream_coder {
ActionRequest DirectStreamCoder::Negotiate() { return ActionRequest::Ready; }

utils::BufferReserveSize DirectStreamCoder::EncodeReserve() const {
  return {0, 0};
}

ActionRequest DirectStreamCoder::Encode(utils::Buffer* buffer) {
  (void)buffer;
  return ActionRequest::Continue;
}

utils::BufferReserveSize DirectStreamCoder::DecodeReserve() const {
  return {0, 0};
}

ActionRequest DirectStreamCoder::Decode(utils::Buffer* buffer) {
  (void)buffer;
  return ActionRequest::Continue;
}

std::error_code DirectStreamCoder::GetLastError() const {
  return std::make_error_code(utils::NEKitErrorCode::NoError);
}

bool DirectStreamCoder::forwarding() const { return true; }

std::unique_ptr<StreamCoderInterface> DirectStreamCoderFactory::Build(
    const utils::Session& session) {
  (void)session;
  return std::make_unique<DirectStreamCoder>();
}
}  // namespace stream_coder
}  // namespace nekit
