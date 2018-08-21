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

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/noncopyable.hpp>

#include "async_interface.h"
#include "cancelable.h"

namespace nekit {
namespace utils {
class Timer : public AsyncInterface, private boost::noncopyable {
 public:
  Timer(utils::Runloop* runloop, std::function<void()> handler);

  ~Timer();

  void Wait(uint32_t milliseconds);
  void Cancel();

  utils::Runloop* GetRunloop() override;

 private:
  utils::Runloop* runloop_;
  std::function<void()> handler_;
  boost::asio::high_resolution_timer timer_;

  Cancelable cancelable_;
};
}  // namespace utils
}  // namespace nekit
