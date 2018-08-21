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

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/noncopyable.hpp>

#include "async_task.h"

namespace nekit {
namespace utils {
class Runloop : private boost::noncopyable {
 public:
  Runloop() = default;

  template <typename... Args>
  void Post(Args&&... args) {
    boost::asio::post(io_context_, std::forward<Args>(args)...);
  }

  void Run() { io_context_.run(); }

  void Stop() { io_context_.stop(); }

  /**
   * @brief Get the underlying boost `io_context`.
   *

   * @return boost::asio::io_context

   * @note Please use this with discretion. Do not relay on boost asio since it
   may be replaced anytime.
   */
  boost::asio::io_context* BoostIoContext() { return &io_context_; }

 private:
  boost::asio::io_context io_context_;
};

template <>
inline void Runloop::Post<std::unique_ptr<AsyncTask>>(
    std::unique_ptr<AsyncTask>&& task) {
  Post([task{std::move(task)}]() { task->Run(); });
}
}  // namespace utils
}  // namespace nekit
