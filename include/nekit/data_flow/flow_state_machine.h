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

#include <boost/assert.hpp>

namespace nekit {
namespace data_flow {

enum class FlowState { Init, Closed, Establishing, Established, Closing };
enum class FlowType { Local, Remote };

class FlowStateMachine final {
 public:
  FlowStateMachine(FlowType type) : type_{type} {};

  bool IsReadable() const {
    return !reading_ &&
           ((type_ == FlowType::Local && state_ == FlowState::Establishing) ||
            state_ == FlowState::Established ||
            (state_ == FlowState::Closing && !read_closed_));
  }

  bool IsWritable() const {
    return !writing_ &&
           ((type_ == FlowType::Local && state_ == FlowState::Establishing) ||
            state_ == FlowState::Established ||
            (state_ == FlowState::Closing && !write_closed_ &&
             !write_closing_));
  }

  bool IsWriteClosable() const { return IsWritable(); }

  bool IsReading() const { return reading_; }

  bool IsWriting() const { return writing_; }

  bool IsReadClosed() const { return read_closed_; }

  bool IsWriteClosed() const { return write_closed_; }

  bool IsWriteClosing() const { return write_closing_; }

  FlowState State() const { return state_; }

  void ConnectBegin() {
    BOOST_ASSERT(state_ == FlowState::Init);
    state_ = FlowState::Establishing;
  }

  void Connected() {
    BOOST_ASSERT(state_ == FlowState::Establishing ||
                 state_ == FlowState::Init);
    state_ = FlowState::Established;
  }

  void ReadBegin() {
    BOOST_ASSERT(IsReadable());
    reading_ = true;
  }

  void ReadEnd() {
    BOOST_ASSERT(reading_);
    reading_ = false;
  }

  void ReadClosed() {
    BOOST_ASSERT(state_ == FlowState::Established ||
                 state_ == FlowState::Closing);
    BOOST_ASSERT(!read_closed_);

    if (state_ == FlowState::Established) {
      read_closed_ = true;
      reading_ = false;
      state_ = FlowState::Closing;
    } else {
      read_closed_ = true;
      reading_ = false;
      if (write_closed_) {
        state_ = FlowState::Closed;
      }
    }
  }

  void WriteBegin() {
    BOOST_ASSERT(IsWritable());
    writing_ = true;
  }

  void WriteEnd() {
    BOOST_ASSERT(writing_);
    writing_ = false;
  }

  void WriteCloseBegin() {
    BOOST_ASSERT(state_ == FlowState::Established ||
                 state_ == FlowState::Closing);
    BOOST_ASSERT(!write_closed_ && !write_closing_);

    state_ = FlowState::Closing;
    writing_ = false;
    write_closing_ = true;
  }

  void WriteCloseEnd() {
    BOOST_ASSERT(write_closing_);
    write_closing_ = false;
    write_closed_ = true;

    if (read_closed_) {
      state_ = FlowState::Closed;
    }
  }

  void Closed() {
    reading_ = false;
    writing_ = false;
    write_closing_ = false;
    write_closed_ = true;
    read_closed_ = true;
    state_ = FlowState::Closed;
  }

  void Errored() { Closed(); }

 private:
  FlowType type_;
  FlowState state_{FlowState::Init};
  bool reading_{false}, writing_{false}, read_closed_{false},
      write_closing_{false}, write_closed_{false};
};
}  // namespace data_flow
}  // namespace nekit
