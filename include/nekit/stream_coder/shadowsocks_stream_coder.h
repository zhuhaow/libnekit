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

#include <boost/asio.hpp>

#include "../crypto/key_generator.h"
#include "nekit/crypto/stream_cipher_interface.h"
#include "stream_coder_interface.h"

namespace nekit {
namespace stream_coder {

class ShadowsocksStreamCoder;

template <template <crypto::Action> class Cipher>
class ShadowsocksStreamCoderFactory : public StreamCoderFactoryInterface {
 public:
  static_assert(!nekit::crypto::is_aead_cipher<
                    Cipher<crypto::Action::Encryption>>::value &&
                    !nekit::crypto::is_aead_cipher<
                        Cipher<crypto::Action::Decryption>>::value,
                "Must not be a cipher using AEAD.");

  ShadowsocksStreamCoderFactory(
      const std::string& key,
      std::shared_ptr<utils::Endpoint> endpoint = nullptr)
      : next_hop_endpoint_{endpoint} {
    size_t key_size = Cipher<crypto::Action::Encryption>{}.key_size();
    size_t iv_size = Cipher<crypto::Action::Encryption>{}.iv_size();

    key_ = static_cast<uint8_t*>(::operator new(key_size));
    crypto::KeyGenerator::ShadowsocksGenerate(
        reinterpret_cast<const uint8_t*>(key.c_str()), key.size(), key_,
        key_size, nullptr, iv_size);
  }

  ShadowsocksStreamCoderFactory(const std::string& key,
                                const std::string& domain, uint16_t port)
      : ShadowsocksStreamCoderFactory{
            key, std::make_shared<utils::Endpoint>(domain, port)} {}

  ShadowsocksStreamCoderFactory(const std::string& key,
                                const boost::asio::ip::address& address,
                                uint16_t port)
      : ShadowsocksStreamCoderFactory{
            key, std::make_shared<utils::Endpoint>(address, port)} {}

  ~ShadowsocksStreamCoderFactory() { free(key_); }

  std::unique_ptr<StreamCoderInterface> Build(
      std::shared_ptr<utils::Session> session) {
    if (next_hop_endpoint_) {
      return std::make_unique<ShadowsocksStreamCoder>(
          next_hop_endpoint_->Dup(),
          std::make_unique<Cipher<crypto::Action::Encryption>>(),
          std::make_unique<Cipher<crypto::Action::Decryption>>(), key_);
    } else {
      return std::make_unique<ShadowsocksStreamCoder>(
          session->endpoint(),
          std::make_unique<Cipher<crypto::Action::Encryption>>(),
          std::make_unique<Cipher<crypto::Action::Decryption>>(), key_);
    }
  }

 private:
  uint8_t* key_;
  std::shared_ptr<utils::Endpoint> next_hop_endpoint_;
};  // namespace stream_coder

class ShadowsocksStreamCoder : public StreamCoderInterface {
 public:
  template <template <crypto::Action> class Cipher>
  using Factory = class ShadowsocksStreamCoderFactory<Cipher>;

  ShadowsocksStreamCoder(
      std::shared_ptr<utils::Endpoint> endpoint,
      std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
      std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
      const uint8_t* key);

  ~ShadowsocksStreamCoder();

  ActionRequest Negotiate() override;

  utils::BufferReserveSize EncodeReserve() const override;
  ActionRequest Encode(utils::Buffer* buffer) override;

  utils::BufferReserveSize DecodeReserve() const override;
  ActionRequest Decode(utils::Buffer* buffer) override;

  std::error_code GetLastError() const override;

  bool forwarding() const override;

 private:
  std::shared_ptr<utils::Endpoint> endpoint_;

  std::unique_ptr<crypto::StreamCipherInterface> encryptor_, decryptor_;
  uint8_t* encryptor_iv_{nullptr};
  bool send_header_{false}, reading_iv_{true};
};

}  // namespace stream_coder
}  // namespace nekit
