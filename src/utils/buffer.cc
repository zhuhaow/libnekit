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

#include <cstdint>

#include "nekit/utils/buffer.h"

namespace nekit {
namespace utils {

Buffer::Buffer(std::size_t size)
    : size_(size), data_(::operator new(size)), front_(0), back_(0) {}

Buffer::~Buffer() { ::operator delete(data_); }

void *Buffer::data() { return data_; }

const void *Buffer::data() const { return data_; }

std::size_t Buffer::size() const { return size_; }

bool Buffer::ReserveFront(std::size_t size) {
  // Be careful. Overflow is not checked.
  if (size >= size_ || size + front_ + back_ > size_) {
    return false;
  }

  front_ += size;
  return true;
}

bool Buffer::ReleaseFront(std::size_t size) {
  if (front_ < size) {
    return false;
  }

  front_ -= size;
  return true;
}

bool Buffer::ReserveBack(std::size_t size) {
  // Be careful. Overflow is not checked.
  if (size >= size_ || size + front_ + back_ > size_) {
    return false;
  }

  back_ += size;
  return true;
}

bool Buffer::ReleaseBack(std::size_t size) {
  if (back_ < size) {
    return false;
  }

  back_ -= size;
  return true;
}

std::size_t Buffer::capacity() const { return size_ - front_ - back_; }

void *Buffer::buffer() { return static_cast<uint8_t *>(data_) + front_; }

const void *Buffer::buffer() const {
  return static_cast<uint8_t *>(data_) + front_;
}

}  // namespace utils
}  // namespace nekit
