#include <glog/logging.h>

#include <nekit/stream_coder/detail/stream_coder_manager.h>
#include <nekit/stream_coder/stream_coder_manager.h>

namespace std {
template <>
struct is_error_code_enum<nekit::stream_coder::StreamCoderManager::ErrorCode>
    : public std::true_type {};

error_code make_error_code(
    nekit::stream_coder::StreamCoderManager::ErrorCode errc) {
  return error_code(static_cast<int>(errc),
                    nekit::stream_coder::StreamCoderManager::error_category());
}
}  // namespace std

namespace nekit {
namespace stream_coder {
namespace detail {

void StreamCoderManager::PrependStreamCoder(
    std::unique_ptr<StreamCoder>&& coder) {
  CHECK_EQ(status_, kInvalid);

  list_.push_back(std::move(coder));
}

utils::Error StreamCoderManager::GetLatestError() const { return last_error_; }

ActionRequest StreamCoderManager::NegotiateNextCoder() {
  CHECK_EQ(status_, kNegotiating);

  while (active_coder_ != list_.end()) {
    auto request = (*active_coder_)->Negotiate();
    switch (request) {
      case kReady:
        ++active_coder_;
        break;
      case kRemoveSelf:
        active_coder_ = list_.erase(active_coder_);
        break;
      case kErrorHappened:
        last_error_ = (*active_coder_)->GetLatestError();
        status_ = kClosed;
        return kErrorHappened;
      case kWantRead:
      case kWantWrite:
        return request;
      case kContinue:
        CHECK(false);  // unreachable
    }
  }

  status_ = kForwarding;
  return kReady;
}

bool StreamCoderManager::VerifyNonEmpty() {
  if (!list_.empty()) return true;

  last_error_ =
      std::make_error_code(stream_coder::StreamCoderManager::kNoCoder);
  return false;
}

ActionRequest StreamCoderManager::Negotiate() {
  CHECK_EQ(status_, kInvalid)
      << ": Cannot start new negotiation on opened StreamCodeManager.";

  if (!VerifyNonEmpty()) {
    return kErrorHappened;
  }

  active_coder_ = list_.begin();
  status_ = kNegotiating;

  return NegotiateNextCoder();
}

StreamCoderManager::StreamCoderIterator StreamCoderManager::FindTailIterator() {
  StreamCoderIterator tail;
  switch (status_) {
    case Phase::kNegotiating:
      CHECK(active_coder_ != list_.end());
      tail = active_coder_;
      tail++;
      break;
    case Phase::kForwarding:
      tail = list_.end();
      break;
    case Phase::kClosed:
    case Phase::kInvalid:
      CHECK(false);  // not reachable
  }

  return tail;
}

BufferReserveSize StreamCoderManager::InputReserve() {
  BufferReserveSize reserve{0, 0};

  auto tail = FindTailIterator();

  for (auto iter = list_.begin(); iter != tail; ++iter) {
    reserve += (*iter)->InputReserve();
  }

  return reserve;
}

ActionRequest StreamCoderManager::Input(utils::Buffer& buffer) {
  if (!VerifyNonEmpty()) {
    return kErrorHappened;
  }

  switch (status_) {
    case kForwarding:
      return InputForForward(buffer);
    case kNegotiating:
      return InputForNegotiation(buffer);
    default:
      CHECK(false);  // not reachable
  }
}

ActionRequest StreamCoderManager::InputForNegotiation(utils::Buffer& buffer) {
  CHECK(active_coder_ != list_.end());

  auto iter = list_.begin();
  while (iter != active_coder_) {
    switch ((*iter)->Input(buffer)) {
      case kContinue:
        ++iter;
        break;
      case kErrorHappened:
        last_error_ = (*iter)->GetLatestError();
        status_ = kClosed;
        return kErrorHappened;
      case kRemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        CHECK(false);  // not reachable
    }
  }

  // now processing active_coder_
  auto action = (*iter)->Input(buffer);
  switch (action) {
    case kErrorHappened:
      last_error_ = (*iter)->GetLatestError();
      status_ = kClosed;
      return kErrorHappened;
    case kRemoveSelf:
      list_.erase(iter);
      ++active_coder_;
      return NegotiateNextCoder();
    case kReady:
      ++active_coder_;
      return NegotiateNextCoder();
    case kWantRead:
    case kWantWrite:
      return action;
    case kContinue:
      CHECK(false);  // not reachable
  }
}

ActionRequest StreamCoderManager::InputForForward(utils::Buffer& buffer) {
  auto iter = list_.begin();
  while (iter != list_.end()) {
    switch ((*iter)->Input(buffer)) {
      case kContinue:
        ++iter;
        break;
      case kErrorHappened:
        last_error_ = (*iter)->GetLatestError();
        status_ = kClosed;
        return kErrorHappened;
      case kRemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        CHECK(false);
    }
  }

  return kContinue;
}

BufferReserveSize StreamCoderManager::OutputReserve() {
  BufferReserveSize reserve{0, 0};

  auto tail = FindTailIterator();

  for (auto iter = list_.begin(); iter != tail; ++iter) {
    reserve += (*iter)->OutputReserve();
  }

  return reserve;
}

ActionRequest StreamCoderManager::Output(utils::Buffer& buffer) {
  if (!VerifyNonEmpty()) {
    return kErrorHappened;
  }

  switch (status_) {
    case kForwarding:
      return OutputForForward(buffer);
    case kNegotiating:
      return OutputForNegotiation(buffer);
    default:
      CHECK(false);  // not reachable
  }
}

ActionRequest StreamCoderManager::OutputForNegotiation(utils::Buffer& buffer) {
  CHECK(active_coder_ != list_.end());

  auto iter = active_coder_;

  // processing active_coder_ first
  auto action = (*iter)->Input(buffer);
  switch (action) {
    case kErrorHappened:
      last_error_ = (*iter)->GetLatestError();
      status_ = kClosed;
      return kErrorHappened;
    case kRemoveSelf:
      ++active_coder_;
      // if the iter is the first one, then the iter will still be the first one
      iter = list_.erase(iter);
      action = NegotiateNextCoder();
      break;
    case kReady:
      ++active_coder_;
      action = NegotiateNextCoder();
      break;
    case kWantRead:
    case kWantWrite:
      break;
    case kContinue:
      CHECK(false);  // not reachable
  }

  if (iter == list_.begin()) {
    return action;
  }

  do {
    --iter;
    switch ((*iter)->Output(buffer)) {
      case kContinue:
        break;
      case kErrorHappened:
        last_error_ = (*iter)->GetLatestError();
        status_ = kClosed;
        return kErrorHappened;
      case kRemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        CHECK(false);  // not reachable
    }
  } while (iter != list_.begin());

  return action;
}

ActionRequest StreamCoderManager::OutputForForward(utils::Buffer& buffer) {
  auto iter = list_.end();

  do {
    --iter;
    switch ((*iter)->Output(buffer)) {
      case kContinue:
        break;
      case kErrorHappened:
        last_error_ = (*iter)->GetLatestError();
        status_ = kClosed;
        return kErrorHappened;
      case kRemoveSelf:
        iter = list_.erase(iter);
        break;
      default:
        CHECK(false);  // not reachable
    }
  } while (iter != list_.begin());

  return kContinue;
}

bool StreamCoderManager::forwarding() const { return status_ == kForwarding; }

}  // namespace detail
}  // namespace stream_coder
}  // namespace nekit
