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

#include "../hedley/hedley.h"
#include <boost/noncopyable.hpp>

#include "../utils/async_interface.h"
#include "../utils/buffer.h"
#include "../utils/cancelable.h"
#include "../utils/result.h"
#include "../utils/session.h"
#include "flow_state_machine.h"

namespace nekit {
namespace data_flow {

enum class DataType { Stream, Packet };

class DataFlowInterface : public utils::AsyncInterface,
                          // This is probably not necessary, but we enforce it
                          // here to avoid any potential error.
                          private boost::noncopyable {
 public:
  virtual ~DataFlowInterface() = default;

  using DataEventHandler = std::function<void(utils::Result<utils::Buffer>&&)>;
  using EventHandler = std::function<void(utils::Result<void>&&)>;

  HEDLEY_WARN_UNUSED_RESULT virtual utils::Cancelable Read(
      DataEventHandler) = 0;
  HEDLEY_WARN_UNUSED_RESULT virtual utils::Cancelable Write(utils::Buffer&&,
                                                            EventHandler) = 0;

  HEDLEY_WARN_UNUSED_RESULT virtual utils::Cancelable CloseWrite(
      EventHandler) = 0;

  virtual const FlowStateMachine& StateMachine() const = 0;

  virtual DataFlowInterface* NextHop() const = 0;

  virtual DataType FlowDataType() const = 0;

  virtual std::shared_ptr<utils::Session> Session() const = 0;
};
}  // namespace data_flow
}  // namespace nekit
