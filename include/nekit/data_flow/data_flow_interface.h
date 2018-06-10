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

#include <boost/noncopyable.hpp>

#include "../utils/async_io_interface.h"
#include "../utils/buffer.h"
#include "../utils/cancelable.h"
#include "../utils/error.h"
#include "../utils/session.h"

#define NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE(__state) \
  (__state == nekit::data_flow::State::Closing)

#define NE_DATA_FLOW_CAN_CHECK_DATA_STATE(__state)    \
  (__state == nekit::data_flow::State::Established || \
   __state == nekit::data_flow::State::Closing)

#define NE_DATA_FLOW_WRITE_CLOSABLE(__data_flow)            \
  (__data_flow->State() == data_flow::State::Established || \
   (__data_flow->State() == data_flow::State::Closing &&    \
    !__data_flow->IsWriteClosed()))

namespace nekit {
namespace data_flow {

enum class DataType { Stream, Packet };

enum class State {
  Closed,
  Establishing,
  Established,
  Closing  // Note even read and write are both closed, it doesn't mean the
           // data flow is ready to be released. Check for the `Closed` flag.
};

class DataFlowInterface : public utils::AsyncIoInterface,
                          // This is probably not necessary, but we enforce it
                          // here to avoid any potential error.
                          private boost::noncopyable {
 public:
  virtual ~DataFlowInterface() = default;

  using DataEventHandler = std::function<void(utils::Buffer&&, utils::Error)>;
  using EventHandler = std::function<void(utils::Error)>;

  virtual utils::Cancelable Read(utils::Buffer&&, DataEventHandler)
      __attribute__((warn_unused_result)) = 0;
  virtual utils::Cancelable Write(utils::Buffer&&, EventHandler)
      __attribute__((warn_unused_result)) = 0;

  // Must not be writing.
  virtual utils::Cancelable CloseWrite(EventHandler)
      __attribute__((warn_unused_result)) = 0;

  // Whether the data flow will read more data, only valid when in state
  // `Closing`. Use `NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE` to check.
  virtual bool IsReadClosed() const = 0;

  // Whether the data flow can send more data, only valid when in state
  // `Closing`. Use `NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE` to check.
  virtual bool IsWriteClosed() const = 0;

  // Whether the socket is closing, only valid in state `Closing`. Use
  // `NE_DATA_FLOW_CAN_CHECK_CLOSE_STATE` to check.
  virtual bool IsWriteClosing() const = 0;

  // Whether the data flow is trying to read data, only valid in state
  // `Established` and `Closing`. Use `NE_DATA_FLOW_CAN_CHECK_DATA_STATE` to
  // check.
  virtual bool IsReading() const = 0;
  // Whether the data flow is writing data, only valid in state `Established`
  // and `Closing`. Use `NE_DATA_FLOW_CAN_CHECK_DATA_STATE` to check.
  virtual bool IsWriting() const = 0;

  virtual data_flow::State State() const = 0;

  virtual DataFlowInterface* NextHop() const = 0;

  virtual DataType FlowDataType() const = 0;

  virtual std::shared_ptr<utils::Session> Session() const = 0;
};
}  // namespace data_flow
}  // namespace nekit
