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

#include <list>
#include <memory>
#include <system_error>

#include <boost/config.hpp>

#include "stream_coder_interface.h"

namespace nekit {
namespace stream_coder {

class StreamCoderPipe final : public StreamCoderInterface {
 public:
  enum class ErrorCode { NoError = 0, NoCoder };

  using StreamCoderIterator =
      std::list<std::unique_ptr<StreamCoderInterface>>::iterator;

  using StreamCoderConstIterator =
      std::list<std::unique_ptr<StreamCoderInterface>>::const_iterator;

  StreamCoderPipe();
  ~StreamCoderPipe() = default;

  void AppendStreamCoder(std::unique_ptr<StreamCoderInterface>&& stream_coder);

  ActionRequest Negotiate() override;

  utils::BufferReserveSize EncodeReserve() const override;
  ActionRequest Encode(utils::Buffer* buffer) override;

  utils::BufferReserveSize DecodeReserve() const override;
  ActionRequest Decode(utils::Buffer* buffer) override;

  std::error_code GetLastError() const override;

  bool forwarding() const override;

 private:
  enum class Phase { Invalid, Negotiating, Forwarding, Closed };

  ActionRequest NegotiateNextCoder();
  bool VerifyNonEmpty();
  StreamCoderConstIterator FindTailIterator() const;
  StreamCoderIterator FindTailIterator();
  ActionRequest EncodeForNegotiation(utils::Buffer* buffer);
  ActionRequest EncodeForForward(utils::Buffer* buffer);
  ActionRequest DecodeForNegotiation(utils::Buffer* buffer);
  ActionRequest DecodeForForward(utils::Buffer* buffer);

  std::list<std::unique_ptr<StreamCoderInterface>> list_;
  StreamCoderIterator active_coder_;
  std::error_code last_error_;
  Phase status_;
};

}  // namespace stream_coder
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::stream_coder::StreamCoderPipe::ErrorCode>
    : public true_type {};

error_code make_error_code(
    nekit::stream_coder::StreamCoderPipe::ErrorCode errc);
}  // namespace std
