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

#include "nekit/stream_coder/shadowsocks_stream_coder_error.h"

#include <string>

namespace nekit {
namespace stream_coder {
namespace {
struct ShadowsocksStreamCoderErrorCategory : std::error_category {
  const char *name() const noexcept override {
    return "Shadowsocks stream coder";
  }

  std::string message(int error_code) const override {
    switch (static_cast<ShadowsocksStreamCoderErrorCode>(error_code)) {
      case ShadowsocksStreamCoderErrorCode::NoError:
        return "no error";
      case ShadowsocksStreamCoderErrorCode::ReadPartialIv:
        return "read partial IV from remote";
      case ShadowsocksStreamCoderErrorCode::InputBufferTooLarge:
        return "input data block is too large";
      case ShadowsocksStreamCoderErrorCode::LengthIsTooLarge:
        return "length is too large";
    }
  }
};

const ShadowsocksStreamCoderErrorCategory shadowsocksStreamCoderErrorCategory{};
}  // namespace

std::error_code make_error_code(ShadowsocksStreamCoderErrorCode ec) {
  return {static_cast<int>(ec), shadowsocksStreamCoderErrorCategory};
}
}  // namespace stream_coder
}  // namespace nekit
