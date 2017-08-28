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
#include <system_error>

#include <boost/noncopyable.hpp>

#include "../utils/buffer.h"
#include "../utils/buffer_reserve_size.h"
#include "../utils/session.h"
#include "action_request.h"

namespace nekit {
namespace stream_coder {

class StreamCoderInterface : boost::noncopyable {
 public:
  virtual ~StreamCoderInterface() = default;

  virtual ActionRequest Negotiate() = 0;

  virtual utils::BufferReserveSize EncodeReserve() const = 0;
  virtual ActionRequest Encode(utils::Buffer* buffer) = 0;

  virtual utils::BufferReserveSize DecodeReserve() const = 0;
  virtual ActionRequest Decode(utils::Buffer* buffer) = 0;

  virtual std::error_code GetLastError() const = 0;

  virtual bool forwarding() const = 0;
};

class StreamCoderFactoryInterface {
 public:
  virtual ~StreamCoderFactoryInterface() = default;

  virtual std::unique_ptr<StreamCoderInterface> Build(
      std::shared_ptr<utils::Session> session) = 0;
};

template <typename StreamCoderType>
class StreamCoderFactory : public StreamCoderFactoryInterface {
 public:
  std::unique_ptr<StreamCoderInterface> Build(
      std::shared_ptr<utils::Session> session) override {
    (void)session;
    return std::make_unique<StreamCoderType>();
  }
};
}  // namespace stream_coder
}  // namespace nekit
