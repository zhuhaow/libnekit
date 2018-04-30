
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

#include <cstdint>

#include <openssl/evp.h>
#include <boost/assert.hpp>

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

    key_ = std::make_unique<uint8_t[]>(key_size());
    iv_ = std::make_unique<uint8_t[]>(iv_size());
  }

  ~OpenSslStreamCipher() { EVP_CIPHER_CTX_free(context_); }

  void SetKey(const void *data) override {
    std::memcpy(key_.get(), data, key_size());
    if (!EVP_CipherInit_ex(context_, nullptr, nullptr, key_.get(), nullptr,
                           static_cast<int>(action_))) {
      exit(1);
    }
  }

  void SetIv(const void *data) override {
    std::memcpy(iv_.get(), data, iv_size());
    if (!EVP_CipherInit_ex(context_, nullptr, nullptr, nullptr, iv_.get(),
                           static_cast<int>(action_))) {
      exit(1);
    }
  }

  ErrorCode Process(const void *input, size_t len, const void *input_tag,
                    void *output, void *output_tag) override {
    int output_len;

    if (action_ == Action::Decryption && input_tag != nullptr) {
      if (!EVP_CIPHER_CTX_ctrl(context_, EVP_CTRL_AEAD_SET_TAG, int(tag_size_),
                               const_cast<void *>(input_tag))) {
        return ErrorCode::UnknownError;
      }
    }

    if (!EVP_CipherUpdate(context_, static_cast<uint8_t *>(output), &output_len,
                          static_cast<const uint8_t*>(input), int(len))) {
      return ErrorCode::UnknownError;
    }
    BOOST_ASSERT(output_len == int(len));

    if (action_ == Action::Decryption && input_tag != nullptr) {
      if (EVP_CipherFinal_ex(context_, static_cast<uint8_t*>(output), &output_len) <= 0) {
        return ErrorCode::ValidationFailed;
      }

      BOOST_ASSERT(output_len == 0);
    } else if (action_ == Action::Encryption && output_tag != nullptr) {
      if (!EVP_CipherFinal_ex(context_, static_cast<uint8_t*>(output), &output_len)) {
        return ErrorCode::UnknownError;
      }

      BOOST_ASSERT(output_len == 0);

      if (!EVP_CIPHER_CTX_ctrl(context_, EVP_CTRL_AEAD_GET_TAG, tag_size_,
                               output_tag)) {
        return ErrorCode::UnknownError;
      }
    }

    return ErrorCode::NoError;
  }

  void Reset() override {
    EVP_CIPHER_CTX_reset(context_);
    key_ = nullptr;
    iv_ = nullptr;

    EVP_CipherInit_ex(context_, type_(), nullptr, nullptr, nullptr,
                      static_cast<int>(action_));
  }

  size_t key_size() override { return EVP_CIPHER_key_length(type_()); }
  size_t iv_size() override { return EVP_CIPHER_iv_length(type_()); }
  size_t block_size() override { return EVP_CIPHER_block_size(type_()); }
  size_t tag_size() override { return tag_size_; }

 private:
  EVP_CIPHER_CTX *context_;
  std::unique_ptr<uint8_t[]> key_, iv_;
};  // namespace crypto

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
template <>
struct is_aead_cipher<ChaCha20IetfPoly1305Cipher<Action::Encryption>>
    : std::true_type {};
template <>
struct is_aead_cipher<ChaCha20IetfPoly1305Cipher<Action::Decryption>>
    : std::true_type {};

template <Action action_>
using Aes128Gcm = OpenSslStreamCipher<action_, EVP_aes_128_gcm, 16>;
template <Action action_>
struct is_aead_cipher<Aes128Gcm<action_>> : std::true_type {};
template <>
struct is_aead_cipher<Aes128Gcm<Action::Decryption>> : std::true_type {};

template <Action action_>
using Aes192Gcm = OpenSslStreamCipher<action_, EVP_aes_192_gcm, 16>;
template <>
struct is_aead_cipher<Aes192Gcm<Action::Encryption>> : std::true_type {};
template <>
struct is_aead_cipher<Aes192Gcm<Action::Decryption>> : std::true_type {};

template <Action action_>
using Aes256Gcm = OpenSslStreamCipher<action_, EVP_aes_256_gcm, 16>;
template <>
struct is_aead_cipher<Aes256Gcm<Action::Encryption>> : std::true_type {};
template <>
struct is_aead_cipher<Aes256Gcm<Action::Decryption>> : std::true_type {};
}  // namespace crypto
}  // namespace nekit
