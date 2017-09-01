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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <boost/noncopyable.hpp>

#include "buffer_reserve_size.h"

namespace nekit {
namespace utils {
struct Buffer final : public boost::noncopyable {
 public:
  explicit Buffer(std::size_t size)
      : capacity_(size),
        data_{std::make_unique<uint8_t[]>(size)},
        front_(0),
        back_(0) {}

  explicit Buffer(const BufferReserveSize &size) : Buffer(size, 0) {}

  Buffer(const BufferReserveSize &size, std::size_t content)
      : Buffer(size.prefix() + size.suffix() + content) {
    ReserveFront(size.prefix());
    ReserveBack(size.suffix());
  }

  Buffer(const Buffer &buffer, std::size_t suffix, std::size_t prefix)
      : capacity_{buffer.size() + suffix + prefix},
        data_{std::make_unique<uint8_t[]>(capacity_)} {
    memcpy(data_.get() + buffer.front_ + suffix,
           buffer.data_.get() + buffer.front_, buffer.size());
    ReserveFront(buffer.front_ + suffix);
    ReserveBack(buffer.back_ + suffix);
  }

  // Return the underlying buffer.
  void *data() { return data_.get(); }
  const void *data() const { return data_.get(); }
  std::size_t capacity() const { return capacity_; }

  bool ReserveFront(std::size_t size) {
    // Be careful. Overflow is not checked.
    if (size + front_ + back_ > capacity_) {
      return false;
    }

    front_ += size;
    return true;
  }

  bool ReleaseFront(std::size_t size) {
    if (front_ < size) {
      return false;
    }

    front_ -= size;
    return true;
  }

  bool ReserveBack(std::size_t size) {
    // Be careful. Overflow is not checked.
    if (size + front_ + back_ > capacity_) {
      return false;
    }

    back_ += size;
    return true;
  }

  bool ReleaseBack(std::size_t size) {
    if (back_ < size) {
      return false;
    }

    back_ -= size;
    return true;
  }

  bool Reset(const BufferReserveSize &reserve_size) {
    if (reserve_size.suffix() + reserve_size.prefix() >= capacity_) {
      return false;
    }

    front_ = reserve_size.prefix();
    back_ = reserve_size.suffix();
    return true;
  }

  void ShrinkSize() { back_ += size(); }

  void Swap(Buffer &rhs) {
    using std::swap;

    swap(capacity_, rhs.capacity_);
    swap(data_, rhs.data_);
    swap(front_, rhs.front_);
    swap(back_, rhs.back_);
  }

  std::size_t size() const { return capacity_ - front_ - back_; }

  void *buffer() { return data_.get() + front_; }

  const void *buffer() const { return data_.get() + front_; }

 private:
  std::size_t capacity_;
  std::unique_ptr<uint8_t[]> data_;

  std::size_t front_;
  std::size_t back_;
};
}  // namespace utils
}  // namespace nekit
