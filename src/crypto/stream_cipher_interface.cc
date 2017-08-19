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

#include <string>

#include "nekit/crypto/stream_cipher_interface.h"

namespace nekit {
namespace crypto {
namespace {
struct StreamCipherErrorCategory : std::error_category {
  const char* name() const noexcept override { return "Stream cipher"; }

  std::string message(int error_code) const override {
    switch (static_cast<StreamCipherInterface::ErrorCode>(error_code)) {
      case StreamCipherInterface::ErrorCode::NoError:
        return "no error";
      case StreamCipherInterface::ErrorCode::ValidationFailed:
        return "data validation failed";
      case StreamCipherInterface::ErrorCode::UnknownError:
        return "unknown error";
    }
  }
};

const StreamCipherErrorCategory streamCipherErrorCategory{};
}  // namespace

std::error_code make_error_code(StreamCipherInterface::ErrorCode ec) {
  return {static_cast<int>(ec), streamCipherErrorCategory};
}
}  // namespace crypto
}  // namespace nekit
