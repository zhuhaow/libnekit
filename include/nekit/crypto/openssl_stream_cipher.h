
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

#include <cassert>
#include <cstdint>

#include <openssl/evp.h>

#include "stream_cipher_interface.h"

namespace nekit {
namespace crypto {

using OpenSslStreamType = const EVP_CIPHER *();

template <Action action_, OpenSslStreamType type_, size_t tag_size_>
class OpenSslStreamCipher : public StreamCipherInterface {
 public:
  OpenSslStreamCipher() {
    if (!(context_ = EVP_CIPHER_CTX_new()) ||
        !EVP_CipherInit_ex(context_, type_(), nullptr, nullptr, nullptr,
                           static_cast<int>(action_))) {
      exit(1);
    }
  }

  ~OpenSslStreamCipher() {
    if (free_key_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(key_)));
    }
    if (free_iv_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(iv_)));
    }

    EVP_CIPHER_CTX_free(context_);
  }

  void SetKey(const uint8_t *data, bool copy) override {
    if (copy) {
      uint8_t *key = static_cast<uint8_t *>(::operator new(key_size()));
      std::memcpy(key, data, key_size());
      free_key_ = true;
      key_ = key;
    } else {
      key_ = data;
    }

    if (!EVP_CipherInit_ex(context_, nullptr, nullptr, key_, nullptr,
                           static_cast<int>(action_))) {
      exit(1);
    }
  }

  void SetIv(const uint8_t *data, bool copy) override {
    if (copy) {
      uint8_t *iv = static_cast<uint8_t *>(::operator new(iv_size()));
      std::memcpy(iv, data, iv_size());
      free_iv_ = true;
      iv_ = iv;
    } else {
      iv_ = data;
    }

    if (!EVP_CipherInit_ex(context_, nullptr, nullptr, nullptr, iv_,
                           static_cast<int>(action_))) {
      exit(1);
    }
  }

  ErrorCode Process(const uint8_t *input, size_t len, const uint8_t *input_tag,
                    uint8_t *output, uint8_t *output_tag) override {
    int output_len;

    if (action_ == Action::Decryption && input_tag != nullptr) {
      if (!EVP_CIPHER_CTX_ctrl(context_, EVP_CTRL_AEAD_SET_TAG, int(tag_size_),
                               const_cast<uint8_t *>(input_tag))) {
        return ErrorCode::UnknownError;
      }
    }

    if (!EVP_CipherUpdate(context_, output, &output_len,
                          const_cast<uint8_t *>(input), int(len))) {
      return ErrorCode::UnknownError;
    }
    assert(output_len == int(len));

    if (action_ == Action::Decryption && input_tag != nullptr) {
      if (EVP_CipherFinal_ex(context_, output, &output_len) <= 0) {
        return ErrorCode::ValidationFailed;
      }

      assert(output_len == 0);
    } else if (action_ == Action::Encryption && output_tag != nullptr) {
      if (!EVP_CipherFinal_ex(context_, output, &output_len)) {
        return ErrorCode::UnknownError;
      }

      assert(output_len == 0);

      if (!EVP_CIPHER_CTX_ctrl(context_, EVP_CTRL_AEAD_GET_TAG, tag_size_,
                               output_tag)) {
        return ErrorCode::UnknownError;
      }
    }

    return ErrorCode::NoError;
  }

  void Reset() override {
    if (free_key_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(key_)));
    }
    if (free_iv_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(iv_)));
    }
    key_ = nullptr;
    iv_ = nullptr;
    free_key_ = false;
    free_iv_ = false;

    EVP_CIPHER_CTX_reset(context_);

    EVP_CipherInit_ex(context_, type_(), nullptr, nullptr, nullptr,
                      static_cast<int>(action_));
  }

  size_t key_size() override { return EVP_CIPHER_key_length(type_()); }
  size_t iv_size() override { return EVP_CIPHER_iv_length(type_()); }
  size_t block_size() override { return EVP_CIPHER_block_size(type_()); }
  size_t tag_size() override { return tag_size_; }

 private:
  EVP_CIPHER_CTX *context_;
  const uint8_t *key_{nullptr}, *iv_{nullptr};
  bool free_key_{false}, free_iv_{false};
};

template <Action action_>
using Aes128CfbCipher = OpenSslStreamCipher<action_, EVP_aes_128_cfb, 0>;

template <Action action_>
using Aes192CfbCipher = OpenSslStreamCipher<action_, EVP_aes_192_cfb, 0>;

template <Action action_>
using Aes256CfbCipher = OpenSslStreamCipher<action_, EVP_aes_256_cfb, 0>;

template <Action action_>
using Aes128CtrCipher = OpenSslStreamCipher<action_, EVP_aes_128_ctr, 0>;

template <Action action_>
using Aes192CtrCipher = OpenSslStreamCipher<action_, EVP_aes_192_ctr, 0>;

template <Action action_>
using Aes256CtrCipher = OpenSslStreamCipher<action_, EVP_aes_256_ctr, 0>;

template <Action action_>
using ChaCha20IetfPoly1305Cipher =
    OpenSslStreamCipher<action_, EVP_chacha20_poly1305, 16>;

template <Action action_>
using Aes128Gcm = OpenSslStreamCipher<action_, EVP_aes_128_gcm, 16>;

template <Action action_>
using Aes192Gcm = OpenSslStreamCipher<action_, EVP_aes_192_gcm, 16>;

template <Action action_>
using Aes256Gcm = OpenSslStreamCipher<action_, EVP_aes_256_gcm, 16>;
}  // namespace crypto
}  // namespace nekit
