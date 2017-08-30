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
#include <list>
#include <string>

#include "nekit/stream_coder/stream_coder_pipe.h"

namespace nekit {
namespace stream_coder {

StreamCoderPipe::StreamCoderPipe() : status_{Phase::Invalid} {}

void StreamCoderPipe::AppendStreamCoder(
    std::unique_ptr<StreamCoderInterface>&& stream_coder) {
  assert(status_ == Phase::Invalid);

  list_.push_back(std::move(stream_coder));
}

std::error_code StreamCoderPipe::GetLastError() const { return last_error_; }

ActionRequest StreamCoderPipe::Negotiate() {
  assert(status_ == Phase::Invalid);

  if (!VerifyNonEmpty()) {
    return ActionRequest::ErrorHappened;
  }

  active_coder_ = list_.begin();
  status_ = Phase::Negotiating;

  return NegotiateNextCoder();
}

utils::BufferReserveSize StreamCoderPipe::EncodeReserve() const {
  utils::BufferReserveSize reserve{0, 0};

  auto tail = FindTailIterator();

  for (auto iter = list_.begin(); iter != tail; ++iter) {
    reserve += (*iter)->EncodeReserve();
  }

  return reserve;
}

ActionRequest StreamCoderPipe::Encode(utils::Buffer* buffer) {
  if (!VerifyNonEmpty()) {
    return ActionRequest::ErrorHappened;
  }

  switch (status_) {
    case Phase::Forwarding:
      return EncodeForForward(buffer);
    case Phase::Negotiating:
      return EncodeForNegotiation(buffer);
    default:
      assert(false);  // not reachable
  }
}

utils::BufferReserveSize StreamCoderPipe::DecodeReserve() const {
  utils::BufferReserveSize reserve{0, 0};

  auto tail = FindTailIterator();

  for (auto iter = list_.begin(); iter != tail; ++iter) {
    reserve += (*iter)->DecodeReserve();
  }

  return reserve;
}

ActionRequest StreamCoderPipe::Decode(utils::Buffer* buffer) {
  if (!VerifyNonEmpty()) {
    return ActionRequest::ErrorHappened;
  }

  switch (status_) {
    case Phase::Forwarding:
      return DecodeForForward(buffer);
    case Phase::Negotiating:
      return DecodeForNegotiation(buffer);
    default:
      assert(false);  // not reachable
  }
}

bool StreamCoderPipe::forwarding() const {
  return status_ == Phase::Forwarding;
}

ActionRequest StreamCoderPipe::NegotiateNextCoder() {
  assert(status_ == Phase::Negotiating);

  while (active_coder_ != list_.end()) {
    auto request = (*active_coder_)->Negotiate();
    switch (request) {
      case ActionRequest::Ready:
        ++active_coder_;
        break;
      case ActionRequest::RemoveSelf:
        active_coder_ = list_.erase(active_coder_);
        break;
      case ActionRequest::ErrorHappened:
        last_error_ = (*active_coder_)->GetLastError();
        status_ = Phase::Closed;
        return ActionRequest::ErrorHappened;
      case ActionRequest::WantRead:
      case ActionRequest::WantWrite:
        return request;
      case ActionRequest::Continue:
      case ActionRequest::Event:
      case ActionRequest::ReadyAfterRead:
        assert(false);  // unreachable
    }
  }

  if (list_.empty()) {
    last_error_ = ErrorCode::NoCoder;
    return ActionRequest::ErrorHappened;
  }

  status_ = Phase::Forwarding;
  return ActionRequest::Ready;
}

bool StreamCoderPipe::VerifyNonEmpty() {
  if (!list_.empty()) {
    return true;
  }

  last_error_ = ErrorCode::NoCoder;
  return false;
}

StreamCoderPipe::StreamCoderConstIterator StreamCoderPipe::FindTailIterator()
    const {
  StreamCoderConstIterator tail;
  switch (status_) {
    case Phase::Negotiating:
      assert(active_coder_ != list_.end());
      tail = active_coder_;
      tail++;
      break;
    case Phase::Forwarding:
      tail = list_.cend();
      break;
    case Phase::Closed:
    case Phase::Invalid:
      assert(false);  // not reachable
  }

  return tail;
}

StreamCoderPipe::StreamCoderIterator StreamCoderPipe::FindTailIterator() {
  StreamCoderIterator tail;
  switch (status_) {
    case Phase::Negotiating:
      assert(active_coder_ != list_.end());
      tail = active_coder_;
      tail++;
      break;
    case Phase::Forwarding:
      tail = list_.end();
      break;
    case Phase::Closed:
    case Phase::Invalid:
      assert(false);  // not reachable
  }

  return tail;
}

ActionRequest StreamCoderPipe::EncodeForNegotiation(utils::Buffer* buffer) {
  assert(active_coder_ != list_.end());

  auto iter = list_.begin();
  while (iter != active_coder_) {
    switch ((*iter)->Encode(buffer)) {
      case ActionRequest::Continue:
        ++iter;
        break;
      case ActionRequest::ErrorHappened:
        last_error_ = (*iter)->GetLastError();
        status_ = Phase::Closed;
        return ActionRequest::ErrorHappened;
      case ActionRequest::RemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        assert(false);  // not reachable
    }
  }

  // now processing active_coder_
  auto action = (*iter)->Encode(buffer);
  switch (action) {
    case ActionRequest::ErrorHappened:
      last_error_ = (*iter)->GetLastError();
      status_ = Phase::Closed;
      return ActionRequest::ErrorHappened;
    case ActionRequest::RemoveSelf:
      list_.erase(iter);
      ++active_coder_;
      return NegotiateNextCoder();
    case ActionRequest::Ready:
      ++active_coder_;
      return NegotiateNextCoder();
    case ActionRequest::WantRead:
    case ActionRequest::WantWrite:
      return action;
    case ActionRequest::Continue:
    case ActionRequest::Event:
    case ActionRequest::ReadyAfterRead:
      assert(false);  // not reachable
  }
}

ActionRequest StreamCoderPipe::EncodeForForward(utils::Buffer* buffer) {
  auto iter = list_.begin();
  while (iter != list_.end()) {
    switch ((*iter)->Encode(buffer)) {
      case ActionRequest::Continue:
        ++iter;
        break;
      case ActionRequest::ErrorHappened:
        last_error_ = (*iter)->GetLastError();
        status_ = Phase::Closed;
        return ActionRequest::ErrorHappened;
      case ActionRequest::RemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        assert(false);
    }
  }

  return ActionRequest::Continue;
}

ActionRequest StreamCoderPipe::DecodeForNegotiation(utils::Buffer* buffer) {
  assert(active_coder_ != list_.end());

  auto iter = active_coder_;

  // processing active_coder_ first
  auto action = (*iter)->Decode(buffer);
  switch (action) {
    case ActionRequest::ErrorHappened:
      last_error_ = (*iter)->GetLastError();
      status_ = Phase::Closed;
      return ActionRequest::ErrorHappened;
    case ActionRequest::RemoveSelf:
      ++active_coder_;
      // if the iter is the first one, then the iter will still be the first
      // one
      iter = list_.erase(iter);
      action = NegotiateNextCoder();
      break;
    case ActionRequest::Ready:
      ++active_coder_;
      action = NegotiateNextCoder();
      break;
    case ActionRequest::WantRead:
    case ActionRequest::WantWrite:
      break;
    case ActionRequest::Continue:
    case ActionRequest::Event:
    case ActionRequest::ReadyAfterRead:
      assert(false);  // not reachable
  }

  if (iter == list_.begin()) {
    return action;
  }

  do {
    --iter;
    switch ((*iter)->Decode(buffer)) {
      case ActionRequest::Continue:
        break;
      case ActionRequest::ErrorHappened:
        last_error_ = (*iter)->GetLastError();
        status_ = Phase::Closed;
        return ActionRequest::ErrorHappened;
      case ActionRequest::RemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        assert(false);  // not reachable
    }
  } while (iter != list_.begin());

  return action;
}

ActionRequest StreamCoderPipe::DecodeForForward(utils::Buffer* buffer) {
  auto iter = list_.end();

  do {
    --iter;
    switch ((*iter)->Decode(buffer)) {
      case ActionRequest::Continue:
        break;
      case ActionRequest::ErrorHappened:
        last_error_ = (*iter)->GetLastError();
        status_ = Phase::Closed;
        return ActionRequest::ErrorHappened;
      case ActionRequest::RemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        assert(false);  // not reachable
    }
  } while (iter != list_.begin());

  return ActionRequest::Continue;
}

namespace {
struct StreamCoderPipeErrorCategory : std::error_category {
  const char* name() const noexcept override;
  std::string message(int) const override;
};

const char* StreamCoderPipeErrorCategory::name() const noexcept {
  return "StreamCoderPipe";
}

std::string StreamCoderPipeErrorCategory::message(int ev) const {
  switch (static_cast<StreamCoderPipe::ErrorCode>(ev)) {
    case StreamCoderPipe::ErrorCode::NoError:
      return "no error";
    case StreamCoderPipe::ErrorCode::NoCoder:
      return "no StreamCoder set";
  }
}

const StreamCoderPipeErrorCategory streamCoderPipeErrorCategory{};

}  // namespace

std::error_code make_error_code(
    nekit::stream_coder::StreamCoderPipe::ErrorCode e) {
  return {static_cast<int>(e), streamCoderPipeErrorCategory};
}
}  // namespace stream_coder
}  // namespace nekit
