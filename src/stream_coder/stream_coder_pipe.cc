#include <list>

#include <nekit/deps/easylogging++.h>

#include <nekit/stream_coder/stream_coder_pipe.h>

namespace nekit {
namespace stream_coder {

class StreamCoderPipe::StreamCoderPipeImplementation {
 public:
  typedef std::list<std::unique_ptr<StreamCoderInterface>>::iterator
      StreamCoderIterator;
  typedef std::list<std::unique_ptr<StreamCoderInterface>>::const_iterator
      StreamCoderConstIterator;

  void AppendStreamCoder(std::unique_ptr<StreamCoderInterface>&& stream_coder) {
    CHECK_EQ(status_, kInvalid);

    list_.push_back(std::move(stream_coder));
  }

  utils::Error GetLatestError() const { return last_error_; }

  ActionRequest Negotiate() {
    CHECK_EQ(status_, kInvalid)
        << ": Cannot start new negotiation on opened StreamCodeManager.";

    if (!VerifyNonEmpty()) {
      return kErrorHappened;
    }

    active_coder_ = list_.begin();
    status_ = kNegotiating;

    return NegotiateNextCoder();
  }

  BufferReserveSize InputReserve() const {
    BufferReserveSize reserve{0, 0};

    auto tail = FindTailIterator();

    for (auto iter = list_.begin(); iter != tail; ++iter) {
      reserve += (*iter)->InputReserve();
    }

    return reserve;
  }

  ActionRequest Input(utils::Buffer& buffer) {
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

    return kErrorHappened;  // not reachable
  }

  BufferReserveSize OutputReserve() const {
    BufferReserveSize reserve{0, 0};

    auto tail = FindTailIterator();

    for (auto iter = list_.begin(); iter != tail; ++iter) {
      reserve += (*iter)->OutputReserve();
    }

    return reserve;
  }

  ActionRequest Output(utils::Buffer& buffer) {
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

    return kErrorHappened;  // not reachable
  }

  bool forwarding() const { return status_ == kForwarding; }

 private:
  enum Phase { kInvalid, kNegotiating, kForwarding, kClosed };

  ActionRequest NegotiateNextCoder() {
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

  bool VerifyNonEmpty() {
    if (!list_.empty()) return true;

    last_error_ = std::make_error_code(stream_coder::kNoCoder);
    return false;
  }

  StreamCoderConstIterator FindTailIterator() const {
    StreamCoderConstIterator tail;
    switch (status_) {
      case Phase::kNegotiating:
        CHECK(active_coder_ != list_.end());
        tail = active_coder_;
        tail++;
        break;
      case Phase::kForwarding:
        tail = list_.cend();
        break;
      case Phase::kClosed:
      case Phase::kInvalid:
        CHECK(false);  // not reachable
    }

    return tail;
  }

  StreamCoderIterator FindTailIterator() {
    const auto iter = static_cast<const StreamCoderPipeImplementation*>(this)
                          ->FindTailIterator();
    return list_.erase(iter, iter);
  }

  ActionRequest InputForNegotiation(utils::Buffer& buffer) {
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

    return kErrorHappened;
  }

  ActionRequest InputForForward(utils::Buffer& buffer) {
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

  ActionRequest OutputForNegotiation(utils::Buffer& buffer) {
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
        // if the iter is the first one, then the iter will still be the first
        // one
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

  ActionRequest OutputForForward(utils::Buffer& buffer) {
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

  std::list<std::unique_ptr<StreamCoderInterface>> list_;
  StreamCoderIterator active_coder_;
  utils::Error last_error_;
  Phase status_;
};

void StreamCoderPipe::AppendStreamCoder(
    std::unique_ptr<StreamCoderInterface>&& stream_coder) {
  impl_->AppendStreamCoder(std::move(stream_coder));
}

utils::Error StreamCoderPipe::GetLatestError() const {
  return impl_->GetLatestError();
}

ActionRequest StreamCoderPipe::Negotiate() { return impl_->Negotiate(); }

BufferReserveSize StreamCoderPipe::InputReserve() const {
  return impl_->InputReserve();
}

ActionRequest StreamCoderPipe::Input(utils::Buffer& buffer) {
  return impl_->Input(buffer);
}

BufferReserveSize StreamCoderPipe::OutputReserve() const {
  return impl_->OutputReserve();
}

ActionRequest StreamCoderPipe::Output(utils::Buffer& buffer) {
  return impl_->Output(buffer);
}

bool StreamCoderPipe::forwarding() const { return impl_->forwarding(); }

}  // namespace stream_coder
}  // namespace nekit
