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

#include "nekit/crypto/key_generator.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>

namespace nekit {
namespace crypto {
void KeyGenerator::ShadowsocksGenerate(const uint8_t *data, size_t data_size,
                                       uint8_t *key, size_t key_size,
                                       uint8_t *iv, size_t iv_size) {
  // The following codes are derived from OpenSSL, but heavily modified.

  EVP_MD_CTX *context = EVP_MD_CTX_new();
  int round = 0;
  uint8_t digest_buffer[EVP_MAX_MD_SIZE];
  unsigned int digest_size = 0;
  size_t key_remain = key_size;
  size_t iv_remain = iv_size;

  if (!context) {
    exit(1);
  }

  while (iv_remain) {
    if (!EVP_DigestInit_ex(context, EVP_md5(), nullptr)) {
      exit(1);
    }

    if (round++) {
      if (!EVP_DigestUpdate(context, digest_buffer, digest_size)) {
        exit(1);
      }
    }

    if (!EVP_DigestUpdate(context, data, data_size)) {
      exit(1);
    }

    if (!EVP_DigestFinal_ex(context, &(digest_buffer[0]), &digest_size)) {
      exit(1);
    }

    auto remain = digest_size;
    while (remain && key_remain) {
      if (key) {
        *key++ = digest_buffer[digest_size - remain];
      }
      remain--;
      key_remain--;
    }

    while (remain && iv_remain) {
      if (iv) {
        *iv++ = digest_buffer[digest_size - remain];
      }
      remain--;
      iv_remain--;
    }
  }

  EVP_MD_CTX_free(context);
}

void KeyGenerator::HkdfGenerate(const uint8_t *data, size_t data_size,
                                const uint8_t *salt, size_t salt_size,
                                const uint8_t *info, size_t info_size,
                                uint8_t *key, size_t key_size, Hash hash_type) {
  EVP_PKEY_CTX *context = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);

  if (EVP_PKEY_derive_init(context) <= 0) {
    exit(1);
  }

  if (EVP_PKEY_CTX_set_hkdf_md(context, ToOpenSslType(hash_type)) <= 0) {
    exit(1);
  }

  if (EVP_PKEY_CTX_set1_hkdf_key(context, data, data_size) <= 0) {
    exit(1);
  }

  if (EVP_PKEY_CTX_set1_hkdf_salt(context, salt, salt_size) <= 0) {
    exit(1);
  }

  if (EVP_PKEY_CTX_add1_hkdf_info(context, info, info_size) <= 0) {
    exit(1);
  }

  if (EVP_PKEY_derive(context, key, &key_size) <= 0) {
    exit(1);
  }

  EVP_PKEY_CTX_free(context);
}
}  // namespace crypto
}  // namespace nekit
