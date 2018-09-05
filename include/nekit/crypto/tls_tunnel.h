// MIT License

// Copyright (c) 2018 Zhuhao Wang

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
#include <string>

#include <openssl/ssl.h>
#include <boost/noncopyable.hpp>

#include "../utils/buffer.h"
#include "../utils/error.h"
#include "../utils/result.h"

namespace nekit {
namespace crypto {

enum class TlsTunnelErrorCode { CommonError = 1 };

class TlsTunnelErrorCategory : public utils::ErrorCategory {
 public:
  using SslErrorCode = unsigned long;

  NE_DEFINE_STATIC_ERROR_CATEGORY(TlsTunnelErrorCategory)

  std::string Description(const utils::Error& error) const override;
  std::string DebugDescription(const utils::Error& error) const override;

  static utils::Error FromSslError(SslErrorCode error_code);
};

class TlsTunnel : private boost::noncopyable {
 public:
  enum class Mode { Server, Client };
  enum class HandShakeAction { WantIo, Success };

  TlsTunnel(std::shared_ptr<SSL_CTX> ctx, Mode mode);

  utils::Result<HandShakeAction> HandShake();

  bool HasPlainTextDataToRead() const;
  utils::Result<utils::Buffer> ReadPlainTextData();
  utils::Result<void> WritePlainTextData(utils::Buffer&& buffer);
  bool FinishWritingCipherData() const;

  bool HasCipherTextDataToRead() const;
  utils::Buffer ReadCipherTextData();
  void WriteCipherTextData(utils::Buffer&& buffer);
  bool NeedCipherInput() const;

  utils::Result<void> SetDomain(const std::string& domain);

  bool Closed() const;

 private:
  void InternalPlainWrite();
  void InternalCipherWrite();
  void FlushPlainBuffer();
  void FlushCipherBuffer();
  void FlushBuffer();

  std::unique_ptr<BIO, decltype(&BIO_free_all)> cipher_bio_;
  std::unique_ptr<SSL, decltype(&SSL_free)> ssl_;

  Mode mode_;

  bool need_cipher_input_{false}, closed_{false};

  utils::Buffer
      pending_write_plain_,  // buffer that should be processed with `SSL_write`
      pending_read_plain_,   // buffer contains data from `SSL_read`
      pending_read_cipher_;  // data should be sent to another peer

  utils::Error error_;
};
}  // namespace crypto
}  // namespace nekit

NE_DEFINE_NEW_ERROR_CODE(TlsTunnel, nekit, crypto)
