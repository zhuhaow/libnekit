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
#include <functional>
#include <memory>
#include <utility>

#include <boost/noncopyable.hpp>

namespace nekit {
namespace utils {

struct Buf;

// The buffer can be empty, which has size 0.
class Buffer final : private boost::noncopyable {
 public:
  Buffer();
  Buffer(size_t size);
  Buffer(Buffer&& buffer);
  ~Buffer();

  bool operator!() const;

  void Insert(Buffer&& buffer, size_t pos);
  void Insert(size_t skip, size_t len);

  void InsertFront(Buffer&& buffer);
  void InsertFront(size_t size);

  void InsertBack(Buffer&& buffer);
  void InsertBack(size_t size);

  void Shrink(size_t from, size_t len);
  void ShrinkFront(size_t size);
  void ShrinkBack(size_t size);

  uint8_t GetByte(size_t skip) const;
  void GetData(size_t skip, size_t len, void* target) const;
  void GetData(size_t skip, size_t len, Buffer* target, size_t offset) const;

  void SetByte(size_t skip, uint8_t data);
  void SetData(size_t skip, size_t len, const void* source);
  void SetData(size_t skip, size_t len, const Buffer& source, size_t offset);

  void WalkInternalChunk(
      const std::function<bool(void* data, size_t len, void* context)>& walker,
      size_t from, void* context);
  void WalkInternalChunk(const std::function<bool(const void* data, size_t len,
                                                  void* context)>& walker,
                         size_t from, void* context) const;

  size_t FindLocation(const void* pointer);

  size_t size() const;

 private:
  void InsertBufAt(Buf* buf, Buffer&& buffer, size_t pos);

  std::unique_ptr<Buf> head_;
  Buf* tail_;
  size_t size_;
};
}  // namespace utils
}  // namespace nekit
