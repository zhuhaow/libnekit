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

#ifndef NEKIT_UTILS_BUFFER
#define NEKIT_UTILS_BUFFER

#include <cstddef>

#include <boost/noncopyable.hpp>

namespace nekit {
namespace utils {
struct Buffer : public boost::noncopyable {
 public:
  Buffer(std::size_t size);
  ~Buffer();

  // Return the underlying buffer.
  void *data();
  const void *data() const;
  std::size_t size() const;

  bool ReserveFront(std::size_t size);
  bool ReleaseFront(std::size_t size);

  bool ReserveBack(std::size_t size);
  bool ReleaseBack(std::size_t size);

  std::size_t capacity() const;
  void *buffer();
  const void *buffer() const;

 private:
  const std::size_t size_;
  void *const data_;

  std::size_t front_;
  std::size_t back_;
};
}  // namespace utils
}  // namespace nekit

#endif /* NEKIT_UTILS_BUFFER */
