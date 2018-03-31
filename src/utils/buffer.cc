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

#include "nekit/utils/buffer.h"

#include <algorithm>
#include <memory>

#include <boost/assert.hpp>

namespace nekit {
namespace utils {

// Buf can't have size 0
struct Buf {
  friend class Buffer;

  std::unique_ptr<int8_t[]> data_;

  size_t capacity_;
  size_t offset_;
  size_t size_;

  Buf* next_buf_;

  Buf(size_t size)
      : data_{std::make_unique<int8_t[]>(size)},
        capacity_{size},
        offset_{0},
        size_{size},
        next_buf_{nullptr} {
    BOOST_ASSERT(size);
  }

  // Let's hope the buf chain won't be too long.
  ~Buf() { delete next_buf_; }

  void Append(Buf* buf) {
    BOOST_ASSERT(buf);

    if (buf == next_buf_) {
      return;
    }

    delete next_buf_;
    next_buf_ = buf;
  }

  Buf* Break(size_t pos) {
    BOOST_ASSERT(pos);

    if (pos == size_) {
      auto temp = next_buf_;
      next_buf_ = nullptr;
      return temp;
    }

    Buf* result = new Buf(size_ - pos);
    std::memcpy(result->data_.get(), data_.get() + offset_ + pos, size_ - pos);
    result->next_buf_ = next_buf_;
    next_buf_ = nullptr;
    size_ -= pos;

    return result;
  }

  void CopyFromMemory(const void* source, size_t pos, size_t len) {
    // Copy only to dest beginning in the current Buf.
    BOOST_ASSERT(pos <= size_);

    auto current = this;
    while (len) {
      BOOST_ASSERT(current);

      size_t copy_len = std::min(len, current->size_ - pos);
      std::memcpy(current->data_.get() + current->offset_ + pos, source,
                  copy_len);
      len -= copy_len;
      current = current->next_buf_;
      source = static_cast<const char*>(source) + copy_len;
      pos = 0;
    }
  }

  void CopyToMemory(void* target, size_t pos, size_t len) {
    // Copy only from source beginning in the current Buf.
    BOOST_ASSERT(pos <= size_);

    auto current = this;
    while (len) {
      BOOST_ASSERT(current);

      size_t copy_len = std::min(len, current->size_ - pos);
      std::memcpy(target, current->data_.get() + current->offset_ + pos,
                  copy_len);
      len -= copy_len;
      current = current->next_buf_;
      target = static_cast<char*>(target) + copy_len;
      pos = 0;
    }
  }
};

Buffer::Buffer(size_t size) {
  if (size) {
    head_ = new Buf(size);
    tail_ = head_;
  } else {
    head_ = tail_ = nullptr;
  }

  size_ = size;
}

Buffer::~Buffer() { delete head_; }

void Buffer::Insert(nekit::utils::Buffer&& buffer, size_t pos) {
  BOOST_ASSERT(pos <= size());
  BOOST_ASSERT(buffer.size());

  if (pos == size()) {
    return InsertBack(std::move(buffer));
  }

  if (pos == 0) {
    return InsertFront(std::move(buffer));
  }

  Buf* current = head_;
  while (pos > current->size_) {
    pos -= current->size_;
    current = current->next_buf_;
  }

  InsertBufAt(current, std::move(buffer), pos);
}

void Buffer::Insert(size_t buf_size, size_t pos) {
  BOOST_ASSERT(pos <= this->size());
  BOOST_ASSERT(buf_size);

  if (pos == size()) {
    return InsertBack(buf_size);
  }

  if (pos == 0) {
    return InsertFront(buf_size);
  }

  Buf* current = head_;
  while (pos > current->size_) {
    pos -= current->size_;
    current = current->next_buf_;
  }

  if (current->size_ == buf_size) {
    // Check if there is already enough space.
    size_t remain = current->capacity_ - current->offset_ - current->size_;
    if (remain >= buf_size ||
        (current->next_buf_ &&
         remain + current->next_buf_->offset_ >= buf_size)) {
      size_ += buf_size;

      current->size_ += std::min(buf_size, remain);
      buf_size -= std::min(buf_size, remain);
      if (buf_size) {
        current->next_buf_->offset_ -= buf_size;
        current->next_buf_->size_ += buf_size;
      }
      return;
    }
  }

  InsertBufAt(current, Buffer(buf_size), pos);
}

void Buffer::InsertFront(nekit::utils::Buffer&& buffer) {
  BOOST_ASSERT(buffer.size());

  auto prev_head = head_;
  head_ = buffer.head_;
  buffer.tail_->next_buf_ = prev_head;
  size_ += buffer.size();

  buffer.ResetWithoutFree();
}

void Buffer::InsertFront(size_t size) {
  if (head_ && head_->offset_ >= size) {
    head_->offset_ -= size;
    head_->size_ += size;
    size_ += size;
    return;
  }

  InsertFront(Buffer(size));
}

void Buffer::InsertBack(Buffer&& buffer) {
  BOOST_ASSERT(buffer.size());

  if (head_) {
    tail_->next_buf_ = buffer.head_;
    tail_ = buffer.tail_;
  } else {
    head_ = buffer.head_;
    tail_ = buffer.tail_;
  }
  size_ += buffer.size();

  buffer.ResetWithoutFree();
}

void Buffer::InsertBack(size_t size) {
  if (tail_ && tail_->capacity_ - tail_->offset_ - tail_->size_ >= size) {
    tail_->size_ += size;
    size_ += size;
    return;
  }

  InsertBack(Buffer(size));
}

void Buffer::Shrink(size_t skip, size_t len) {
  BOOST_ASSERT(skip < size());
  BOOST_ASSERT(len < size());
  BOOST_ASSERT(skip + len < size());

  Buf *current = head_, *prev = nullptr;
  while (skip >= current->size_) {
    skip -= current->size_;
    prev = current;
    current = current->next_buf_;
  }

  if (skip) {
    if (len >= current->size_ - skip) {
      current->size_ = skip;
      len -= skip;
      prev = current;
      current = current->next_buf_;
      skip = 0;
    } else {
      std::memmove(current->data_.get() + current->offset_ + skip,
                   current->data_.get() + current->offset_ + skip + len,
                   current->size_ - skip - len);
      current->size_ -= skip;
      return;
    }
  }

  while (len) {
    auto remove_length = std::min(len, current->size_);
    current->offset_ += remove_length;
    current->size_ -= remove_length;

    if (current->size_) {
      return;
    }

    len -= remove_length;

    if (!prev) {
      head_ = current->next_buf_;
      current = current->next_buf_;
    } else {
      prev->next_buf_ = current->next_buf_;
      current = current->next_buf_;
    }
  }

  if (!current) {
    tail_ = prev;
  }
}

void Buffer::ShrinkFront(size_t size) { Shrink(0, size); }

void Buffer::ShrinkBack(size_t size) {
  // It can be optimized if the Buf is a double linked list.
  Shrink(this->size() - size, size);
}

uint8_t Buffer::GetByte(size_t skip) const {
  BOOST_ASSERT(skip <= size());

  auto current = head_;

  while (skip >= current->size_) {
    skip -= current->size_;
    current = current->next_buf_;
  }

  return *(current->data_.get() + skip);
}

void Buffer::GetData(size_t skip, size_t len, void* target) const {
  BOOST_ASSERT(skip <= size());
  BOOST_ASSERT(len <= size());
  BOOST_ASSERT(skip + len <= size());

  auto current = head_;

  while (skip >= current->size_) {
    skip -= current->size_;
    current = current->next_buf_;
  }

  current->CopyToMemory(target, skip, len);
}

void Buffer::GetData(size_t skip, size_t len, nekit::utils::Buffer* target,
                     size_t offset) const {
  BOOST_ASSERT(skip <= size());
  BOOST_ASSERT(len <= size());
  BOOST_ASSERT(skip + len <= size());

  BOOST_ASSERT(offset <= target->size());
  BOOST_ASSERT(len <= target->size());
  BOOST_ASSERT(offset + len <= target->size());

  auto tcurrent = target->head_;
  while (offset >= tcurrent->size_) {
    offset -= tcurrent->size_;
    tcurrent = tcurrent->next_buf_;
  }

  auto scurrent = head_;
  while (skip >= scurrent->size_) {
    skip -= scurrent->size_;
    scurrent = scurrent->next_buf_;
  }

  while (len) {
    size_t copy_len = std::min(scurrent->size_ - skip,
                               std::min(len, tcurrent->size_ - offset));
    std::memcpy(tcurrent->data_.get() + tcurrent->offset_ + offset,
                scurrent->data_.get() + scurrent->offset_ + skip, copy_len);

    skip += copy_len;
    offset += copy_len;
    len -= copy_len;

    if (skip == scurrent->size_) {
      scurrent = scurrent->next_buf_;
      skip = 0;
    }
    if (offset == tcurrent->size_) {
      tcurrent = tcurrent->next_buf_;
      offset = 0;
    }
  }
}

void Buffer::SetByte(size_t skip, uint8_t data) {
  BOOST_ASSERT(skip <= size());

  auto current = head_;

  while (skip >= current->size_) {
    skip -= current->size_;
    current = current->next_buf_;
  }

  *(current->data_.get() + skip) = data;
}

void Buffer::SetData(size_t skip, size_t len, const void* source) {
  BOOST_ASSERT(skip <= size());
  BOOST_ASSERT(len <= size());
  BOOST_ASSERT(skip + len <= size());

  auto current = head_;

  while (skip >= current->size_) {
    skip -= current->size_;
    current = current->next_buf_;
  }

  current->CopyFromMemory(source, skip, len);
}

void Buffer::SetData(size_t skip, size_t len,
                     const nekit::utils::Buffer& source, size_t offset) {
  source.GetData(offset, len, this, skip);
}

void Buffer::WalkInternalChunk(
    const std::function<bool(void*, size_t, void*)>& walker, size_t from,
    void* context) {
  BOOST_ASSERT(from <= size());

  Buf* current = head_;

  while (from >= current->size_) {
    from -= current->size_;
    current = current->next_buf_;
  }

  while (current) {
    if (!walker(current->data_.get() + from, current->size_ - from, context)) {
      return;
    }
    from = 0;
    current = current->next_buf_;
  }
}

size_t Buffer::size() const { return size_; }

// pos may be the size of buf
void Buffer::InsertBufAt(Buf* buf, Buffer&& buffer, size_t pos) {
  Buf* new_buf = buf->Break(pos);
  buf->next_buf_ = buffer.head_;
  buffer.tail_->next_buf_ = new_buf;

  if (buf == tail_) {
    if (new_buf) {
      tail_ = new_buf;
    } else {
      tail_ = buffer.tail_;
    }
  }

  size_ += buffer.size();

  buffer.ResetWithoutFree();
}

void Buffer::ResetWithoutFree() {
  head_ = nullptr;
  tail_ = nullptr;
  size_ = 0;
}

}  // namespace utils
}  // namespace nekit
