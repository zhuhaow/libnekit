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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include <sodium/crypto_stream_chacha20.h>
#include <sodium/crypto_stream_salsa20.h>
#include <sodium/crypto_stream_xchacha20.h>
#include <sodium/crypto_stream_xsalsa20.h>

#include "stream_cipher_interface.h"

namespace nekit {
namespace crypto {
template <typename BlockCounterLengthType>
using SodiumStreamMethodType = int(unsigned char *, const unsigned char *,
                                   unsigned long long, const unsigned char *,
                                   BlockCounterLengthType,
                                   const unsigned char *);

// Chacha20 and Salsa20 use a nonce instead of IV, we still use iv here for
// interface consistency.
template <Action action_, typename BlockCounterLengthType,
          SodiumStreamMethodType<BlockCounterLengthType> method_,
          size_t key_size_, size_t iv_size_, size_t block_size_>
class SodiumStreamCipher : public StreamCipherInterface {
 public:
  SodiumStreamCipher() {}

  void SetKey(const void *data) override {
    std::memcpy(key_.get(), data, key_size_);
  }

  void SetIv(const void *data) override {
    std::memcpy(iv_.get(), data, iv_size_);
  }

  utils::Result<void> Process(const void *input, size_t len,
                              const void *input_tag, void *output,
                              void *output_tag) override {
    (void)input_tag;
    (void)output_tag;

    // Padding the content to the first block
    size_t content_size =
        std::min(static_cast<size_t>((-counter) % block_size_), len);
    if (content_size != 0) {
      uint8_t *buf = block_buffer_.data() + block_size_ - content_size;
      std::memcpy(buf, input, content_size);
      method_(block_buffer_.data(), block_buffer_.data(), block_size_, iv_,
              counter / block_size_, key_);
      std::memcpy(output, buf, content_size);
      output = static_cast<uint8_t *>(output) + content_size;
      counter += content_size;
      input = static_cast<uint8_t *>(output) + content_size;
      len -= content_size;
    }

    // Process all left over content
    method_(output, input, len, iv_.data(), counter / block_size_, key_.data());
    counter += len;
    return {};
  }

  void Reset() override {
    counter = 0;
    key_ = nullptr;
    iv_ = nullptr;
  }

  size_t key_size() override { return key_size_; }
  size_t iv_size() override { return iv_size_; }
  size_t block_size() override { return block_size_; }
  size_t tag_size() override { return 0; }

 private:
  uint64_t counter{0};

  std::array<uint8_t, block_size_> block_buffer_;
  std::array<uint8_t, key_size_> key_;
  std::array<uint8_t, iv_size_> iv_;
};

template <Action action_>
using ChaCha20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_chacha20_xor_ic,
                       crypto_stream_chacha20_KEYBYTES,
                       crypto_stream_chacha20_NONCEBYTES, 64>;

template <>
struct is_block_cipher<ChaCha20Cipher<Action::Decryption>> : std::true_type {};
template <>
struct is_block_cipher<ChaCha20Cipher<Action::Encryption>> : std::true_type {};

template <Action action_>
using ChaCha20IetfCipher =
    SodiumStreamCipher<action_, uint32_t, crypto_stream_chacha20_ietf_xor_ic,
                       crypto_stream_chacha20_IETF_KEYBYTES,
                       crypto_stream_chacha20_IETF_NONCEBYTES, 64>;
template <>
struct is_block_cipher<ChaCha20IetfCipher<Action::Decryption>>
    : std::true_type {};
template <>
struct is_block_cipher<ChaCha20IetfCipher<Action::Encryption>>
    : std::true_type {};

template <Action action_>
using XChaCha20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_xchacha20_xor_ic,
                       crypto_stream_xchacha20_KEYBYTES,
                       crypto_stream_xchacha20_NONCEBYTES, 64>;
template <>
struct is_block_cipher<XChaCha20Cipher<Action::Decryption>> : std::true_type {};
template <>
struct is_block_cipher<XChaCha20Cipher<Action::Encryption>> : std::true_type {};

template <Action action_>
using Salsa20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_salsa20_xor_ic,
                       crypto_stream_salsa20_KEYBYTES,
                       crypto_stream_salsa20_NONCEBYTES, 64>;
template <>
struct is_block_cipher<Salsa20Cipher<Action::Decryption>> : std::true_type {};
template <>
struct is_block_cipher<Salsa20Cipher<Action::Encryption>> : std::true_type {};

template <Action action_>
using XSalsa20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_xsalsa20_xor_ic,
                       crypto_stream_xsalsa20_KEYBYTES,
                       crypto_stream_xsalsa20_NONCEBYTES, 64>;
template <>
struct is_block_cipher<XSalsa20Cipher<Action::Decryption>> : std::true_type {};
template <>
struct is_block_cipher<XSalsa20Cipher<Action::Encryption>> : std::true_type {};

}  // namespace crypto
}  // namespace nekit
