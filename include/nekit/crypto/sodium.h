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
#include <cstdint>
#include <cstring>
#include <new>

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
  SodiumStreamCipher() {
    block_buffer_ = static_cast<uint8_t *>(::operator new(block_size_));
  }

  ~SodiumStreamCipher() {
    ::operator delete(block_buffer_);

    if (free_key_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(key_)));
    }
    if (free_iv_) {
      ::operator delete(
          const_cast<void *>(reinterpret_cast<const void *>(iv_)));
    }
  }

  void SetKey(const uint8_t *data, bool copy) override {
    if (copy) {
      uint8_t *key = static_cast<uint8_t *>(::operator new(key_size_));
      std::memcpy(key, data, key_size_);
      free_key_ = true;
      key_ = key;
    } else {
      key_ = data;
    }
  }

  void SetIv(const uint8_t *data, bool copy) override {
    if (copy) {
      uint8_t *iv = static_cast<uint8_t *>(::operator new(iv_size_));
      std::memcpy(iv, data, iv_size_);
      free_iv_ = true;
      iv_ = iv;
    } else {
      iv_ = data;
    }
  }

  ErrorCode Process(const uint8_t *input, size_t len, const uint8_t *input_tag,
                    uint8_t *output, uint8_t *output_tag) override {
    (void)input_tag;
    (void)output_tag;

    // Padding the content to the first block
    size_t content_size =
        std::min(static_cast<size_t>((-counter) % block_size_), len);
    if (content_size != 0) {
      uint8_t *buf = block_buffer_ + block_size_ - content_size;
      std::memcpy(buf, input, content_size);
      method_(block_buffer_, block_buffer_, block_size_, iv_,
              counter / block_size_, key_);
      std::memcpy(output, buf, content_size);
      output += content_size;
      counter += content_size;
      input += content_size;
      len -= content_size;
    }

    // Process all left over content
    method_(output, input, len, iv_, counter / block_size_, key_);
    counter += len;
    return ErrorCode::NoError;
  }

  void Reset() override {
    counter = 0;
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
  }

  size_t key_size() override { return key_size_; }
  size_t iv_size() override { return iv_size_; }
  size_t block_size() override { return block_size_; }
  size_t tag_size() override { return 0; }

 private:
  uint64_t counter{0};
  const uint8_t *key_{nullptr}, *iv_{nullptr};
  bool free_key_{false}, free_iv_{false};

  uint8_t *block_buffer_;
};

template <Action action_>
using ChaCha20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_chacha20_xor_ic,
                       crypto_stream_chacha20_KEYBYTES,
                       crypto_stream_chacha20_NONCEBYTES, 64>;

template <Action action_>
using ChaCha20IetfCipher =
    SodiumStreamCipher<action_, uint32_t, crypto_stream_chacha20_ietf_xor_ic,
                       crypto_stream_chacha20_KEYBYTES,
                       crypto_stream_chacha20_NONCEBYTES, 64>;

template <Action action_>
using XChaCha20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_xchacha20_xor_ic,
                       crypto_stream_xchacha20_KEYBYTES,
                       crypto_stream_xchacha20_NONCEBYTES, 64>;

template <Action action_>
using Salsa20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_salsa20_xor_ic,
                       crypto_stream_salsa20_KEYBYTES,
                       crypto_stream_salsa20_NONCEBYTES, 64>;

template <Action action_>
using XSalsa20Cipher =
    SodiumStreamCipher<action_, uint64_t, crypto_stream_xsalsa20_xor_ic,
                       crypto_stream_xsalsa20_KEYBYTES,
                       crypto_stream_xsalsa20_NONCEBYTES, 64>;

}  // namespace crypto
}  // namespace nekit
