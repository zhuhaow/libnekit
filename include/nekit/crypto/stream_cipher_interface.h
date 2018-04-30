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

#include <memory>
#include <system_error>

#include <boost/noncopyable.hpp>

namespace nekit {
namespace crypto {

enum class Action { Decryption = 0, Encryption = 1 };

// This class provide support for stream cipher or block cipher in stream mode.
class StreamCipherInterface : private boost::noncopyable {
 public:
  enum class ErrorCode { NoError, ValidationFailed, UnknownError };

  virtual ~StreamCipherInterface() = default;

  virtual void SetKey(const void *data) = 0;
  virtual void SetIv(const void *data) = 0;

  virtual ErrorCode Process(const void *input, size_t len,
                            const void *input_tag, void *output,
                            void *output_tag) = 0;

  virtual void Reset() = 0;

  virtual size_t key_size() = 0;
  virtual size_t iv_size() = 0;
  virtual size_t block_size() = 0;
  virtual size_t tag_size() = 0;
};

std::error_code make_error_code(StreamCipherInterface::ErrorCode ec);

template <typename Cipher>
struct is_aead_cipher : std::false_type {};

template <typename Cipher>
struct is_block_cipher : std::false_type {};
}  // namespace crypto
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::crypto::StreamCipherInterface::ErrorCode>
    : true_type {};
}  // namespace std
