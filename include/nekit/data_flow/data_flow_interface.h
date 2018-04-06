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

#include <functional>
#include <memory>
#include <system_error>

#include <boost/noncopyable.hpp>

#include "../utils/async_io_interface.h"
#include "../utils/buffer.h"
#include "../utils/cancelable.h"
#include "../utils/session.h"

namespace nekit {
namespace data_flow {

enum class DataType { Stream, Packet };

class DataFlowInterface : public utils::AsyncIoInterface,
                          // This is probably not necessary, but we enforce it
                          // here to avoid any potential error.
                          private boost::noncopyable {
 public:
  virtual ~DataFlowInterface() = default;

  using DataEventHandler =
      std::function<void(std::unique_ptr<utils::Buffer>&&, std::error_code)>;
  using EventHandler = std::function<void(std::error_code)>;

  virtual const utils::Cancelable& Read(std::unique_ptr<utils::Buffer>&&,
                                        DataEventHandler)
      __attribute__((warn_unused_result)) = 0;
  virtual const utils::Cancelable& Write(std::unique_ptr<utils::Buffer>&&,
                                         EventHandler)
      __attribute__((warn_unused_result)) = 0;

  // This should cancel the current write request if there is one.
  virtual const utils::Cancelable& CloseWrite(EventHandler)
      __attribute__((warn_unused_result)) = 0;

  // Whether the data flow will read more data.
  virtual bool IsReadClosed() const = 0;
  // Whether the data flow can send more data.
  virtual bool IsWriteClosed() const = 0;
  // Whether the data flow is still open. This does not mean the data flow
  // should be released, there may be actions pending. Check `IsIdle` also.
  virtual bool IsClosed() const = 0;

  // Whether the data flow is trying to read data.
  virtual bool IsReading() const = 0;
  // Whether the data flow is writing data.
  virtual bool IsWriting() const = 0;

  // Whether there is pending action.
  virtual bool IsIdle() const = 0;

  virtual DataFlowInterface* NextHop() const = 0;

  virtual DataType FlowDataType() const = 0;

  virtual std::shared_ptr<utils::Session> session() const = 0;
};
}  // namespace data_flow
}  // namespace nekit
