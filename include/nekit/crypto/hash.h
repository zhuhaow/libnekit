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

#include <openssl/evp.h>

namespace nekit {
namespace crypto {
enum class Hash { MD4, MD5, SHA1, SHA224, SHA256, SHA384, SHA512 };

inline const EVP_MD* ToOpenSslType(Hash type) {
  switch (type) {
    case Hash::MD4:
      return EVP_md4();
    case Hash::MD5:
      return EVP_md5();
    case Hash::SHA1:
      return EVP_sha1();
    case Hash::SHA224:
      return EVP_sha224();
    case Hash::SHA256:
      return EVP_sha256();
    case Hash::SHA384:
      return EVP_sha384();
    case Hash::SHA512:
      return EVP_sha512();
  }
}
}  // namespace crypto
}  // namespace nekit
