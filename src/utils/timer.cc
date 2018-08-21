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

#include "nekit/utils/timer.h"

namespace nekit {
namespace utils {

Timer::Timer(utils::Runloop* runloop, std::function<void()> handler)
    : runloop_{runloop},
      handler_{handler},
      timer_{*runloop->BoostIoContext()} {}

Timer::~Timer() { Cancel(); }

void Timer::Wait(uint32_t milliseconds) {
  timer_.expires_after(std::chrono::milliseconds(milliseconds));

  cancelable_.Cancel();
  cancelable_.Reset();

  timer_.async_wait(
      [this, cancelable{cancelable_}](const boost::system::error_code& ec) {
        if (cancelable.canceled()) {
          return;
        }

        if (ec) {
          return;
        }

        handler_();
      });
}

void Timer::Cancel() {
  timer_.cancel();
  cancelable_.Cancel();
}

utils::Runloop* Timer::GetRunloop() { return runloop_; }
}  // namespace utils
}  // namespace nekit
