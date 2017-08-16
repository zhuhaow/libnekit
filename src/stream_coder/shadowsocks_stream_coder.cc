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

#include "nekit/stream_coder/shadowsocks_stream_coder.h"

#include "nekit/crypto/random.h"
#include "nekit/utils/error.h"

namespace nekit {
namespace stream_coder {
ShadowsocksStreamCoder::ShadowsocksStreamCoder(
    const std::string& domain, uint16_t port,
    std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
    std::unique_ptr<crypto::StreamCipherInterface>&& decryptor, const uint8_t* key)
    : domain_{domain},
      port_{port},
      encryptor_{std::move(encryptor)},
      decryptor_{std::move(decryptor)} {
  assert(domain_.size() <= 255);
  encryptor_->SetKey(key, true);
  decryptor_->SetKey(key, true);
}

ShadowsocksStreamCoder::ShadowsocksStreamCoder(
    const boost::asio::ip::address& address, uint16_t port,
    std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
    std::unique_ptr<crypto::StreamCipherInterface>&& decryptor, const uint8_t* key)
    : ShadowsocksStreamCoder{address.to_string(), port, std::move(encryptor),
                             std::move(decryptor), key} {}

ShadowsocksStreamCoder::~ShadowsocksStreamCoder() {
  if (encryptor_iv_) {
    ::operator delete(encryptor_iv_);
  }
}

ActionRequest ShadowsocksStreamCoder::Negotiate() {
  return ActionRequest::Ready;
}

utils::BufferReserveSize ShadowsocksStreamCoder::EncodeReserve() const {
  if (send_header_) {
    return {0, 0};
  }

  return {encryptor_->iv_size() + 1 + 1 + domain_.size() + 2, 0};
}

ActionRequest ShadowsocksStreamCoder::Encode(utils::Buffer* buffer) {
  if (!send_header_) {
    encryptor_iv_ =
        static_cast<uint8_t*>(::operator new(encryptor_->iv_size()));
    crypto::Random::Bytes(encryptor_iv_, encryptor_->iv_size());
    encryptor_->SetIv(encryptor_iv_, false);

    buffer->ReleaseFront(encryptor_->iv_size() + 1 + 1 + domain_.size() + 2);
    uint8_t* buf = static_cast<uint8_t*>(buffer->buffer());

    memcpy(buffer->buffer(), encryptor_iv_, encryptor_->iv_size());
    buf += encryptor_->iv_size();

    *buf++ = 3;
    *buf++ = uint8_t(domain_.size());
    memcpy(buf, domain_.c_str(), domain_.size());
    buf += domain_.size();
    *reinterpret_cast<uint16_t*>(buf) = htons(port_);

    buf = static_cast<uint8_t*>(buffer->buffer());
    buf += encryptor_->iv_size();
    encryptor_->Process(buf, buffer->size() - encryptor_->iv_size(), nullptr,
                        buf, nullptr);

    send_header_ = true;
  } else {
    uint8_t* buf = static_cast<uint8_t*>(buffer->buffer());
    encryptor_->Process(buf, buffer->size(), nullptr, buf, nullptr);
  }

  return ActionRequest::Continue;
}

utils::BufferReserveSize ShadowsocksStreamCoder::DecodeReserve() const {
  return {0, 0};
}

ActionRequest ShadowsocksStreamCoder::Decode(utils::Buffer* buffer) {
  if (reading_iv_) {
    assert(buffer->size() >= decryptor_->iv_size());
    decryptor_->SetIv(static_cast<uint8_t*>(buffer->buffer()), true);
    buffer->ReserveFront(decryptor_->iv_size());
    reading_iv_ = false;
  }

  uint8_t* buf = static_cast<uint8_t*>(buffer->buffer());
  decryptor_->Process(buf, buffer->size(), nullptr, buf, nullptr);
  return ActionRequest::Continue;
}

std::error_code ShadowsocksStreamCoder::GetLastError() const {
  return utils::NEKitErrorCode::NoError;
}

bool ShadowsocksStreamCoder::forwarding() const { return true; }

}  // namespace stream_coder
}  // namespace nekit
