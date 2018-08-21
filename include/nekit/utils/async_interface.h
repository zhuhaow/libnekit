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

#include "runloop.h"

namespace nekit {
namespace utils {

/**
 * @brief Interface any class with asynchronous methods should inherit.
 *
 * @note It must be guaranteed that the `Runloop` will not be released before
 * any instance that will return that `Runloop` is released.
 */
class AsyncInterface {
 public:
  virtual ~AsyncInterface() = default;

  /**
   * @brief Get the underlying `Runloop`.
   *
   * @return The `Runloop`.
   */
  virtual Runloop* GetRunloop() = 0;
};

}  // namespace utils
}  // namespace nekit
