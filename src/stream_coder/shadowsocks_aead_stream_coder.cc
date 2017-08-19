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

#include "nekit/stream_coder/shadowsocks_aead_stream_coder.h"

#include <cassert>

#include <boost/endian/conversion.hpp>

#include "nekit/crypto/key_generator.h"
#include "nekit/crypto/random.h"
#include "nekit/stream_coder/shadowsocks_stream_coder_error.h"

namespace nekit {
namespace stream_coder {

ShadowsocksAeadStreamCoder::ShadowsocksAeadStreamCoder(
    const std::string& domain, uint16_t port,
    std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
    std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
    const uint8_t* key)
    : domain_{domain},
      port_{port},
      encryptor_{std::move(encryptor)},
      decryptor_{std::move(decryptor)} {
  key_ = std::make_unique<uint8_t[]>(encryptor_->key_size());
  memcpy(key_.get(), key, encryptor_->key_size());

  encryptor_nonce_ = std::make_unique<uint8_t[]>(encryptor_->iv_size());

  encryptor_salt_ = std::make_unique<uint8_t[]>(encryptor_->key_size());
  crypto::Random::Bytes(encryptor_salt_.get(), encryptor_->key_size());

  encryptor_derived_key_ = std::make_unique<uint8_t[]>(encryptor_->key_size());

  crypto::KeyGenerator::HkdfGenerate(
      key_.get(), encryptor_->key_size(), encryptor_salt_.get(),
      encryptor_->key_size(), reinterpret_cast<const uint8_t*>("ss-subkey"), 9,
      encryptor_derived_key_.get(), encryptor_->key_size(), crypto::Hash::SHA1);

  decryptor_nonce_ = std::make_unique<uint8_t[]>(decryptor_->iv_size());

  decryptor_derived_key_ = std::make_unique<uint8_t[]>(decryptor_->key_size());

  pending_encrypt_data_ =
      std::make_unique<uint8_t[]>(0x3FFF + decryptor_->tag_size());

  pending_length_data_ =
      std::make_unique<uint8_t[]>(decryptor_->tag_size() + 2);
}

ShadowsocksAeadStreamCoder::ShadowsocksAeadStreamCoder(
    const boost::asio::ip::address& address, uint16_t port,
    std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
    std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
    const uint8_t* key)
    : ShadowsocksAeadStreamCoder(address.to_string(), port,
                                 std::move(encryptor), std::move(decryptor),
                                 key) {}

ActionRequest ShadowsocksAeadStreamCoder::Negotiate() {
  return ActionRequest::Ready;
}

utils::BufferReserveSize ShadowsocksAeadStreamCoder::EncodeReserve() const {
  if (send_header_) {
    return {2 + encryptor_->tag_size(), encryptor_->tag_size()};
  }

  // salt and key have same size.
  return {encryptor_->key_size() + 2 + encryptor_->tag_size() + 1 + 1 +
              domain_.size() + 2,
          encryptor_->tag_size()};
}

ActionRequest ShadowsocksAeadStreamCoder::Encode(utils::Buffer* buffer) {
  if (buffer->size() >
      0x3FFF - (2 + encryptor_->tag_size() + 1 + 1 + domain_.size() + 2)) {
    last_error_ = ShadowsocksStreamCoderErrorCode::InputBufferTooLarge;
    return ActionRequest::ErrorHappened;
  }

  // Pointer to data to be encrypted.
  uint8_t* data;
  // Length of data to be encrypted.
  size_t data_len = buffer->size();

  // Pointer to the beginning of the data block.
  uint8_t* buffer_head;

  if (!send_header_) {
    buffer->ReleaseFront(encryptor_->key_size() + 2 + encryptor_->tag_size() +
                         1 + 1 + domain_.size() + 2);
    buffer->ReleaseBack(encryptor_->tag_size());

    memcpy(buffer->buffer(), encryptor_salt_.get(), encryptor_->key_size());

    data = static_cast<uint8_t*>(buffer->buffer()) + encryptor_->key_size() +
           2 + encryptor_->tag_size();

    *data++ = 3;
    *data++ = uint8_t(domain_.size());
    memcpy(data, domain_.c_str(), domain_.size());
    data += domain_.size();
    *reinterpret_cast<uint16_t*>(data) = htons(port_);

    data = static_cast<uint8_t*>(buffer->buffer()) + encryptor_->key_size() +
           2 + encryptor_->tag_size();
    data_len += 2 + domain_.size() + 2;

    buffer_head =
        static_cast<uint8_t*>(buffer->buffer()) + encryptor_->key_size();

    send_header_ = true;
  } else {
    data = static_cast<uint8_t*>(buffer->buffer());

    buffer->ReleaseFront(2 + encryptor_->tag_size());
    buffer->ReleaseBack(encryptor_->tag_size());

    buffer_head = static_cast<uint8_t*>(buffer->buffer());
  }

  encryptor_->SetKey(encryptor_derived_key_.get(), false);
  encryptor_->SetIv(encryptor_nonce_.get(), false);

  uint16_t l = htons(uint16_t(data_len));
  encryptor_->Process(reinterpret_cast<uint8_t*>(&l), 2, nullptr, buffer_head,
                      buffer_head + 2);
  BumpEncryptorNonce();
  encryptor_->Reset();

  encryptor_->SetKey(encryptor_derived_key_.get(), false);
  encryptor_->SetIv(encryptor_nonce_.get(), false);

  buffer_head += 2 + encryptor_->tag_size();
  encryptor_->Process(data, data_len, nullptr, buffer_head,
                      buffer_head + data_len);
  BumpEncryptorNonce();
  encryptor_->Reset();

  return ActionRequest::Continue;
}

utils::BufferReserveSize ShadowsocksAeadStreamCoder::DecodeReserve() const {
  return {pending_encrypt_data_size_, 0};
}

ActionRequest ShadowsocksAeadStreamCoder::Decode(utils::Buffer* buffer) {
  if (reading_salt_) {
    if (buffer->size() < decryptor_->key_size()) {
      last_error_ = ShadowsocksStreamCoderErrorCode::ReadPartialIv;
      return ActionRequest::ErrorHappened;
    }

    crypto::KeyGenerator::HkdfGenerate(
        key_.get(), decryptor_->key_size(),
        static_cast<uint8_t*>(buffer->buffer()), decryptor_->key_size(),
        reinterpret_cast<const uint8_t*>("ss-subkey"), 9,
        decryptor_derived_key_.get(), decryptor_->key_size(),
        crypto::Hash::SHA1);

    buffer->ReserveFront(decryptor_->key_size());
    reading_salt_ = false;
  }

  uint8_t* data = static_cast<uint8_t*>(buffer->buffer());
  size_t remain = buffer->size();

  buffer->ReleaseFront(pending_encrypt_data_size_);
  uint8_t* output = static_cast<uint8_t*>(buffer->buffer());

  while (remain) {
    if (!next_encrypt_data_size_) {
      if (pending_length_data_size_ || remain < 2 + decryptor_->tag_size()) {
        size_t append_length = std::min(
            remain, 2 + decryptor_->tag_size() - pending_length_data_size_);

        memcpy(pending_length_data_.get() + pending_length_data_size_, data,
               append_length);
        data += append_length;
        remain -= append_length;
        pending_length_data_size_ += append_length;

        if (pending_length_data_size_ == 2 + decryptor_->tag_size()) {
          uint16_t next_block_length;

          decryptor_->SetKey(decryptor_derived_key_.get(), false);
          decryptor_->SetIv(decryptor_nonce_.get(), false);
          last_error_ = decryptor_->Process(
              pending_length_data_.get(), 2, pending_length_data_.get() + 2,
              reinterpret_cast<uint8_t*>(&next_block_length), nullptr);
          if (last_error_) {
            return ActionRequest::ErrorHappened;
          }
          BumpDecryptorNonce();
          decryptor_->Reset();

          next_encrypt_data_size_ = ntohs(next_block_length);
            if (next_encrypt_data_size_ > 0x3FFF) {
                last_error_ = ShadowsocksStreamCoderErrorCode::LengthIsTooLarge;
                return ActionRequest::ErrorHappened;
            }
          pending_length_data_size_ = 0;
        }
      } else {
        uint16_t next_block_length;

        decryptor_->SetKey(decryptor_derived_key_.get(), false);
        decryptor_->SetIv(decryptor_nonce_.get(), false);
        last_error_ = decryptor_->Process(
            data, 2, data + 2, reinterpret_cast<uint8_t*>(&next_block_length),
            nullptr);
        if (last_error_) {
          return ActionRequest::ErrorHappened;
        }
        BumpDecryptorNonce();
        decryptor_->Reset();

          next_encrypt_data_size_ = ntohs(next_block_length);
          if (next_encrypt_data_size_ > 0x3FFF) {
              last_error_ = ShadowsocksStreamCoderErrorCode::LengthIsTooLarge;
              return ActionRequest::ErrorHappened;
          }
        data += 2 + decryptor_->tag_size();
        remain -= 2 + decryptor_->tag_size();
      }
    } else {
      if (pending_encrypt_data_size_ ||
          remain < next_encrypt_data_size_ + decryptor_->tag_size()) {
        size_t append_length =
            std::min(remain, next_encrypt_data_size_ + decryptor_->tag_size() -
                                 pending_encrypt_data_size_);

        memcpy(pending_encrypt_data_.get() + pending_encrypt_data_size_, data,
               append_length);
        data += append_length;
        remain -= append_length;
        pending_encrypt_data_size_ += append_length;

        if (pending_encrypt_data_size_ ==
            next_encrypt_data_size_ + decryptor_->tag_size()) {
          decryptor_->SetKey(decryptor_derived_key_.get(), false);
          decryptor_->SetIv(decryptor_nonce_.get(), false);
          last_error_ = decryptor_->Process(
              pending_encrypt_data_.get(), next_encrypt_data_size_,
              pending_encrypt_data_.get() + next_encrypt_data_size_, output,
              nullptr);
          if (last_error_) {
            return ActionRequest::ErrorHappened;
          }
          BumpDecryptorNonce();
          decryptor_->Reset();

          output += next_encrypt_data_size_;
          pending_encrypt_data_size_ = 0;
          next_encrypt_data_size_ = 0;
        }
      } else {
        decryptor_->SetKey(decryptor_derived_key_.get(), false);
        decryptor_->SetIv(decryptor_nonce_.get(), false);
        if (data == output) {
          // We can only do it inplace when it's perfect aligned. OpenSSL will
          // check and enforce it.
          last_error_ = decryptor_->Process(data, next_encrypt_data_size_,
                                            data + next_encrypt_data_size_,
                                            output, nullptr);
        } else {
          memcpy(pending_encrypt_data_.get(), data, next_encrypt_data_size_);
          last_error_ = decryptor_->Process(
              pending_encrypt_data_.get(), next_encrypt_data_size_,
              data + next_encrypt_data_size_, output, nullptr);
        }
        if (last_error_) {
          return ActionRequest::ErrorHappened;
        }
        BumpDecryptorNonce();
        decryptor_->Reset();

        output += next_encrypt_data_size_;
        data += next_encrypt_data_size_ + decryptor_->tag_size();
        remain -= next_encrypt_data_size_ + decryptor_->tag_size();
        next_encrypt_data_size_ = 0;
      }
    }
  }

  buffer->ReserveBack(static_cast<uint8_t*>(buffer->buffer()) + buffer->size() -
                      output);

  return ActionRequest::Continue;
}

std::error_code ShadowsocksAeadStreamCoder::GetLastError() const {
  return last_error_;
}

bool ShadowsocksAeadStreamCoder::forwarding() const { return true; }

void ShadowsocksAeadStreamCoder::BumpEncryptorNonce() {
  int64_t* count = reinterpret_cast<int64_t*>(encryptor_nonce_.get());
  *count = boost::endian::native_to_little(
      boost::endian::little_to_native(*count) + 1);
}

void ShadowsocksAeadStreamCoder::BumpDecryptorNonce() {
  int64_t* count = reinterpret_cast<int64_t*>(decryptor_nonce_.get());
  *count = boost::endian::native_to_little(
      boost::endian::little_to_native(*count) + 1);
}

}  // namespace stream_coder
}  // namespace nekit
