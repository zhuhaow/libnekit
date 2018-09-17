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

#include "nekit/crypto/tls_tunnel.h"

#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <boost/assert.hpp>

#include "nekit/third_party/hedley/hedley.h"

#include "nekit/config.h"

#define NE_TLS_ERROR_MESSAGE_LENGTH 1024

namespace nekit {
namespace crypto {

std::string TlsTunnelErrorCategory::Description(
    const utils::Error& error) const {
  char buf[NE_TLS_ERROR_MESSAGE_LENGTH];
  ERR_error_string_n(error.ErrorCode(), buf, NE_TLS_ERROR_MESSAGE_LENGTH);
  return buf;
}

std::string TlsTunnelErrorCategory::DebugDescription(
    const utils::Error& error) const {
  return utils::ErrorCategory::Description(error);
}

utils::Error TlsTunnelErrorCategory::FromSslError(SslErrorCode error_code) {
  return utils::Error(TlsTunnelErrorCategory::GlobalTlsTunnelErrorCategory(),
                      (int)error_code);
}

TlsTunnel::TlsTunnel(std::shared_ptr<SSL_CTX> ctx, Mode mode)
    : cipher_bio_{nullptr, BIO_free_all}, ssl_{nullptr, SSL_free}, mode_{mode} {
  SSL* ssl = SSL_new(ctx.get());
  // Here we see the problem with C style error handling, we don't know what
  // error may happen so we can do nothing about it. Here we output the error
  // stack then terminate. This is not right way to do things, but there is
  // nothing we can do to improve.
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  ssl_ = std::unique_ptr<SSL, decltype(&SSL_free)>(ssl, SSL_free);

  if (mode_ == Mode::Server) {
    SSL_set_accept_state(ssl_.get());
  } else {
    SSL_set_connect_state(ssl_.get());
  }

  BIO *cipher_buffer, *plain_buffer;

  if (!BIO_new_bio_pair(&cipher_buffer, 0, &plain_buffer, 0)) {
    // Wrong way again.
    ERR_print_errors_fp(stderr);
    exit(1);
  }
  cipher_bio_ = std::unique_ptr<BIO, decltype(&BIO_free_all)>(cipher_buffer,
                                                              BIO_free_all);
  SSL_set_bio(ssl_.get(), plain_buffer, plain_buffer);
}

bool TlsTunnel::HasPlainTextDataToRead() const {
  return (bool)pending_read_plain_;
}

utils::Result<utils::Buffer> TlsTunnel::ReadPlainTextData() {
  // Aka, SSL_read

  if (error_) {
    return utils::MakeErrorResult(std::move(error_));
  }

  return std::move(pending_read_plain_);
}

utils::Result<void> TlsTunnel::WritePlainTextData(utils::Buffer&& buffer) {
  // Aka, SSL_write

  BOOST_ASSERT(FinishWritingCipherData());

  pending_write_plain_ = std::move(buffer);
  InternalPlainWrite();

  if (error_) {
    return utils::MakeErrorResult(std::move(error_));
  }

  return {};
}

bool TlsTunnel::FinishWritingCipherData() const {
  return !pending_write_plain_ && !pending_read_cipher_;
}

bool TlsTunnel::HasCipherTextDataToRead() const {
  // Error is also readable
  return (bool)pending_read_cipher_ || error_;
}

utils::Buffer TlsTunnel::ReadCipherTextData() {
  return std::move(pending_read_cipher_);
}

void TlsTunnel::WriteCipherTextData(utils::Buffer&& buffer) {
  size_t buffer_processed = 0;
  while (buffer_processed != buffer.size() && !error_) {
    buffer.WalkInternalChunk(
        [this, &buffer_processed](const void* data, size_t len, void* context) {
          (void)context;

          need_cipher_input_ = false;
          int n = BIO_write(cipher_bio_.get(), data, len);

          if (n <= 0) {  // error happened
            if (!BIO_should_retry(cipher_bio_.get())) {
              error_ = TlsTunnelErrorCategory::FromSslError(ERR_get_error());
              return false;
            }
            return false;
          }

          buffer_processed += (size_t)n;
          if ((size_t)n == len) {
            return true;
          } else {
            return false;
          }
        },
        buffer_processed, nullptr);

    if (!error_) {
      FlushBuffer();
    }
  }

  return;
}

bool TlsTunnel::NeedCipherInput() const { return need_cipher_input_; }

utils::Result<void> TlsTunnel::SetDomain(const std::string& domain) {
  SSL_set_hostflags(ssl_.get(), X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  if (!SSL_set1_host(ssl_.get(), domain.c_str())) {
    return utils::MakeErrorResult(
        TlsTunnelErrorCategory::FromSslError(ERR_get_error()));
  }
  SSL_set_verify(ssl_.get(), SSL_VERIFY_PEER, nullptr);
  return {};
}

utils::Result<TlsTunnel::HandShakeAction> TlsTunnel::HandShake() {
  int n =
      mode_ == Mode::Server ? SSL_accept(ssl_.get()) : SSL_connect(ssl_.get());
  switch (SSL_get_error(ssl_.get(), n)) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:  // Not possible
      FlushCipherBuffer();
      return HandShakeAction::WantIo;
    case SSL_ERROR_NONE:
      return HandShakeAction::Success;
    default:
      return utils::MakeErrorResult(
          TlsTunnelErrorCategory::FromSslError(ERR_get_error()));
  }
}

void TlsTunnel::InternalPlainWrite() {
  size_t writed = 0;

  pending_write_plain_.WalkInternalChunk(
      [this, &writed](const void* data, size_t data_len, void* context) {
        (void)context;

        int n = SSL_write(ssl_.get(), data, data_len);
        if ((size_t)n == data_len) {
          writed += n;
          return true;
        } else if (n > 0) {
          BOOST_ASSERT((size_t)n < data_len);
          writed += n;
          return false;
        }

        switch (SSL_get_error(ssl_.get(), n)) {
          case SSL_ERROR_WANT_READ:
            need_cipher_input_ = true;
            return false;
          case SSL_ERROR_WANT_WRITE:
            // Buffer is full
            return false;
          case SSL_ERROR_ZERO_RETURN:
            closed_ = true;
            return false;
          default:
            error_ = TlsTunnelErrorCategory::FromSslError(ERR_get_error());
            return false;
        }
      },
      0, nullptr);

  pending_write_plain_.ShrinkFront(writed);

  FlushCipherBuffer();

  if (pending_write_plain_ && !need_cipher_input_ && !closed_ && !error_) {
    InternalPlainWrite();
  }
}

void TlsTunnel::FlushPlainBuffer() {
  size_t offset = pending_read_plain_.size();
  bool stop = false;
  do {
    pending_read_plain_.InsertBack(NEKIT_TLS_READ_SIZE);
    pending_read_plain_.WalkInternalChunk(
        [this, &offset, &stop](void* data, size_t len, void* context) {
          (void)context;

          int l = SSL_read(ssl_.get(), data, len);
          if (l <= 0) {
            stop = true;
            switch (SSL_get_error(ssl_.get(), l)) {
              case SSL_ERROR_WANT_READ:
              case SSL_ERROR_WANT_WRITE:
                return false;
              case SSL_ERROR_ZERO_RETURN:
                closed_ = true;
                return false;
              default:
                error_ = TlsTunnelErrorCategory::FromSslError(ERR_get_error());
                return false;
            }
          } else if ((size_t)l < len) {
            stop = true;
            offset += l;
            return false;
          } else if ((size_t)l == len) {
            offset += l;
            return true;
          } else {
            HEDLEY_UNREACHABLE_RETURN(false);
          }
        },
        offset, nullptr);
  } while (!stop);

  pending_read_plain_.ShrinkBack(pending_read_plain_.size() - offset);
}

void TlsTunnel::FlushCipherBuffer() {
  size_t pending = BIO_ctrl_pending(cipher_bio_.get());

  if (pending) {
    size_t offset = 0;
    if (!pending_read_cipher_) {
      pending_read_cipher_ = utils::Buffer(pending);
    } else {
      offset = pending_read_cipher_.size();
      pending_read_cipher_.InsertBack(pending);
    }

    pending_read_cipher_.WalkInternalChunk(
        [this](void* data, size_t len, void* context) {
          (void)context;

          // This conversion makes sense since len can't be ULONG_MAX
          BOOST_VERIFY((size_t)BIO_read(cipher_bio_.get(), data, len) == len);
          return true;
        },
        offset, nullptr);
  }
}

void TlsTunnel::FlushBuffer() {
  FlushCipherBuffer();
  FlushPlainBuffer();
}

bool TlsTunnel::Closed() const { return closed_; }

}  // namespace crypto
}  // namespace nekit
