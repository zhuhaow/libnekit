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

#include "../crypto/key_generator.h"
#include "../crypto/stream_cipher_interface.h"
#include "stream_coder_interface.h"

namespace nekit {
namespace stream_coder {
class ShadowsocksAeadStreamCoder : public StreamCoderInterface {
 public:
  ShadowsocksAeadStreamCoder(
      const std::string& domain, uint16_t port,
      std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
      std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
      const uint8_t* key);
  ShadowsocksAeadStreamCoder(
      const boost::asio::ip::address& address, uint16_t port,
      std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
      std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
      const uint8_t* key);

  ActionRequest Negotiate() override;

  utils::BufferReserveSize EncodeReserve() const override;
  ActionRequest Encode(utils::Buffer* buffer) override;

  utils::BufferReserveSize DecodeReserve() const override;
  ActionRequest Decode(utils::Buffer* buffer) override;

  std::error_code GetLastError() const override;

  bool forwarding() const override;

 private:
  void BumpEncryptorNonce();
  void WriteRemainEncodeData(uint8_t* data);

  void BumpDecryptorNonce();

  std::string domain_;
  uint16_t port_;

  std::unique_ptr<uint8_t[]> key_;

  std::unique_ptr<crypto::StreamCipherInterface> encryptor_;
  std::unique_ptr<uint8_t[]> encryptor_salt_;
  std::unique_ptr<uint8_t[]> encryptor_derived_key_;
  std::unique_ptr<uint8_t[]> encryptor_nonce_;
  bool send_header_{false};

  std::unique_ptr<crypto::StreamCipherInterface> decryptor_;
  std::unique_ptr<uint8_t[]> decryptor_derived_key_;
  std::unique_ptr<uint8_t[]> decryptor_nonce_;
  std::unique_ptr<uint8_t[]> pending_length_data_;
  size_t pending_length_data_size_{0};
  std::unique_ptr<uint8_t[]> pending_encrypt_data_;
  size_t pending_encrypt_data_size_{0};
  size_t next_encrypt_data_size_{0};
  bool reading_salt_{true};

  std::error_code last_error_;
};

template <template <crypto::Action action_> class Cipher>
class ShadowsocksAeadStreamCoderFactory : public StreamCoderFactoryInterface {
 public:
  ShadowsocksAeadStreamCoderFactory(const std::string& key) {
    size_t key_size = Cipher<crypto::Action::Encryption>{}.key_size();
    size_t iv_size = Cipher<crypto::Action::Encryption>{}.iv_size();

    key_ = std::make_unique<uint8_t[]>(key_size);
    crypto::KeyGenerator::ShadowsocksGenerate(
        reinterpret_cast<const uint8_t*>(key.c_str()), key.size(), key_.get(),
        key_size, nullptr, iv_size);
  }

  std::unique_ptr<StreamCoderInterface> Build(
      std::shared_ptr<utils::Session> session) {
    switch (session->type()) {
      case utils::Session::Type::Address:
        return std::make_unique<ShadowsocksAeadStreamCoder>(
            session->address(), session->port(),
            std::make_unique<Cipher<crypto::Action::Encryption>>(),
            std::make_unique<Cipher<crypto::Action::Decryption>>(), key_.get());
      case utils::Session::Type::Domain:
        return std::make_unique<ShadowsocksAeadStreamCoder>(
            session->domain()->domain(), session->port(),
            std::make_unique<Cipher<crypto::Action::Encryption>>(),
            std::make_unique<Cipher<crypto::Action::Decryption>>(), key_.get());
    }
  }

 private:
  std::unique_ptr<uint8_t[]> key_;
};
}  // namespace stream_coder
}  // namespace nekit
