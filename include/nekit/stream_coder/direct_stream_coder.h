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

#include "stream_coder_interface.h"

namespace nekit {
namespace stream_coder {
class DirectStreamCoder : public StreamCoderInterface {
 public:
  ActionRequest Negotiate();

  utils::BufferReserveSize EncodeReserve() const;
  ActionRequest Encode(utils::Buffer* buffer);

  utils::BufferReserveSize DecodeReserve() const;
  ActionRequest Decode(utils::Buffer* buffer);

  std::error_code GetLastError() const;

  bool forwarding() const;
};

class DirectStreamCoderFactory : public StreamCoderFactoryInterface {
 public:
  std::unique_ptr<StreamCoderInterface> Build(const utils::Session& session) override;
};
}  // namespace stream_coder
}  // namespace nekit
