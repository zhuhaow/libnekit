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

#include "nekit/utils/error.h"

#define SSL_READ_SIZE 8192

#define ENSURE_VALID(x)                                 \
  do {                                                  \
    if (!(x)) {                                         \
      throw nekit::utils::NEKitErrorCode::GeneralError; \
    }                                                   \
  } while (0)

namespace nekit {
namespace crypto {
TlsTunnel::TlsTunnel(std::shared_ptr<SSL_CTX> ctx, Mode mode)
    : cipher_bio_{nullptr, BIO_free_all}, ssl_{nullptr, SSL_free}, mode_{mode} {
  SSL* ssl = SSL_new(ctx.get());
  ENSURE_VALID(ssl);
  ssl_ = std::unique_ptr<SSL, decltype(&SSL_free)>(ssl, SSL_free);

  if (mode_ == Mode::Server) {
    SSL_set_accept_state(ssl_.get());
  } else {
    SSL_set_connect_state(ssl_.get());
  }

  BIO *cipher_buffer, *plain_buffer;
  ENSURE_VALID(BIO_new_bio_pair(&cipher_buffer, 0, &plain_buffer, 0));
  cipher_bio_ = std::unique_ptr<BIO, decltype(&BIO_free_all)>(cipher_buffer,
                                                              BIO_free_all);
  SSL_set_bio(ssl_.get(), plain_buffer, plain_buffer);
}

bool TlsTunnel::HasPlainTextDataToRead() const {
  return (bool)pending_read_plain_;
}

utils::Buffer TlsTunnel::ReadPlainTextData() {
  // Aka, SSL_read

  return std::move(pending_read_plain_);
}

void TlsTunnel::WritePlainTextData(utils::Buffer&& buffer) {
  // Aka, SSL_write

  BOOST_ASSERT(FinishWritingCipherData());

  pending_write_plain_ = std::move(buffer);
  InternalPlainWrite();
}

bool TlsTunnel::FinishWritingCipherData() const {
  return !pending_write_plain_ && !pending_read_cipher_;
}

bool TlsTunnel::HasCipherTextDataToRead() const {
  return (bool)pending_read_cipher_;
}

utils::Buffer TlsTunnel::ReadCipherTextData() {
  return std::move(pending_read_cipher_);
}

void TlsTunnel::WriteCipherTextData(utils::Buffer&& buffer) {
  size_t buffer_processed = 0;
  while (buffer_processed != buffer.size() && !errored_) {
    buffer.WalkInternalChunk(
        [this, &buffer_processed](const void* data, size_t len, void* context) {
          (void)context;

          need_cipher_input_ = false;
          int n = BIO_write(cipher_bio_.get(), data, len);

          if (n <= 0) {  // error happened
            if (!BIO_should_retry(cipher_bio_.get())) {
              errored_ = true;
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

    if (!errored_) {
      FlushBuffer();
    }
  }
}

bool TlsTunnel::NeedCipherInput() const { return need_cipher_input_; }

void TlsTunnel::SetDomain(const std::string& domain) {
  SSL_set_hostflags(ssl_.get(), X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  ENSURE_VALID(SSL_set1_host(ssl_.get(), domain.c_str()));
  SSL_set_verify(ssl_.get(), SSL_VERIFY_PEER, nullptr);
}

TlsTunnel::HandShakeAction TlsTunnel::HandShake() {
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
      return HandShakeAction::Error;
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
            errored_ = true;
            return false;
        }
      },
      0, nullptr);

  pending_write_plain_.ShrinkFront(writed);

  FlushCipherBuffer();

  if (pending_write_plain_ && !need_cipher_input_ && !closed_ && !errored_) {
    InternalPlainWrite();
  }
}

void TlsTunnel::FlushPlainBuffer() {
  size_t offset = pending_read_plain_.size();
  bool stop = false;
  do {
    pending_read_plain_.InsertBack(SSL_READ_SIZE);
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
              default:
                errored_ = true;
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
            BOOST_ASSERT(false);
            return false;
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

bool TlsTunnel::Errored() const { return errored_; }

}  // namespace crypto
}  // namespace nekit
