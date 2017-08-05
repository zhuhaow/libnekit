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

#include <cassert>

#include "nekit/utils/cancelable.h"

namespace nekit {
namespace utils {
Cancelable::Cancelable()
    : canceled_{std::make_shared<bool>(false)},
      count_{std::make_shared<int>(1)} {}

Cancelable::Cancelable(const Cancelable& cancelable) {
  // This is definitely programming error.
  assert(*cancelable.count_ == 1);

  canceled_ = cancelable.canceled_;
  count_ = cancelable.count_;
  (*count_)++;
}

Cancelable& Cancelable::operator=(const Cancelable& cancelable) {
  // This is definitely programming error.
  assert(*cancelable.count_ == 1);

  canceled_ = cancelable.canceled_;
  count_ = cancelable.count_;
  (*count_)++;
  return *this;
}

Cancelable::Cancelable(Cancelable&& cancelable) {
  // We can't move the original flag.
  assert(*cancelable.count_ >= 2);

  cancelable.canceled_.swap(canceled_);
  cancelable.count_.swap(count_);
}

Cancelable& Cancelable::operator=(Cancelable&& cancelable) {
  // We can't move the origin flag.
  assert(*cancelable.count_ == 2);

  cancelable.canceled_.swap(canceled_);
  cancelable.count_.swap(count_);
  return *this;
}

Cancelable::~Cancelable() { Cancel(); }

void Cancelable::Cancel() { *canceled_ = true; }

bool Cancelable::canceled() const { return *canceled_; }

}  // namespace utils
}  // namespace nekit
