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

#include <boost/asio/deadline_timer.hpp>
#include <boost/noncopyable.hpp>

#include "async_io_interface.h"
#include "cancelable.h"

namespace nekit {
namespace utils {
class Timer : public AsyncIoInterface, private LifeTime {
 public:
  Timer(boost::asio::io_context* io, std::function<void()> handler);

  void Wait(uint32_t seconds);
  void Cancel();
  boost::asio::io_context* io();

 private:
  boost::asio::io_context* io_;
  std::function<void()> handler_;
  boost::asio::deadline_timer timer_;
};
}  // namespace utils
}  // namespace nekit
