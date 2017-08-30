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
    std::shared_ptr<utils::Endpoint> endpoint,
    std::unique_ptr<crypto::StreamCipherInterface>&& encryptor,
    std::unique_ptr<crypto::StreamCipherInterface>&& decryptor,
    const uint8_t* key)
    : endpoint_{endpoint},
      encryptor_{std::move(encryptor)},
      decryptor_{std::move(decryptor)} {
  encryptor_->SetKey(key, true);
  decryptor_->SetKey(key, true);
}

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

  return {encryptor_->iv_size() + 1 + 1 + endpoint_->host().size() + 2, 0};
}

ActionRequest ShadowsocksStreamCoder::Encode(utils::Buffer* buffer) {
  if (!send_header_) {
    encryptor_iv_ =
        static_cast<uint8_t*>(::operator new(encryptor_->iv_size()));
    crypto::Random::Bytes(encryptor_iv_, encryptor_->iv_size());
    encryptor_->SetIv(encryptor_iv_, false);

    buffer->ReleaseFront(encryptor_->iv_size() + 1 + 1 +
                         endpoint_->host().size() + 2);
    uint8_t* buf = static_cast<uint8_t*>(buffer->buffer());

    memcpy(buffer->buffer(), encryptor_iv_, encryptor_->iv_size());
    buf += encryptor_->iv_size();

    *buf++ = 3;
    *buf++ = uint8_t(endpoint_->host().size());
    memcpy(buf, endpoint_->host().c_str(), endpoint_->host().size());
    buf += endpoint_->host().size();
    *reinterpret_cast<uint16_t*>(buf) = htons(endpoint_->port());

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
