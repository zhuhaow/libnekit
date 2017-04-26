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

const char* StreamCoderManager::ErrorCategory::name() const noexcept {
  return "NEKit::StreamCoderManager";
}

std::string StreamCoderManager::ErrorCategory::message(int error_code) const {
  switch (ErrorCode(error_code)) {
    case ErrorCode::kNoCoder:
      return "No StreamCoder set.";
  }
}

const StreamCoderManager::ErrorCategory& StreamCoderManager::error_category() {
  static ErrorCategory category_;
  return category_;
}

void StreamCoderManager::PrependStreamCoder(
    std::unique_ptr<StreamCoder>&& coder) {
  impl_->PrependStreamCoder(std::move(coder));
}

ActionRequest StreamCoderManager::Negotiate() { return impl_->Negotiate(); }

BufferReserveSize StreamCoderManager::InputReserve() {
  return impl_->InputReserve();
}

ActionRequest StreamCoderManager::Input(utils::Buffer& buffer) {
  return impl_->Input(buffer);
}

BufferReserveSize StreamCoderManager::OutputReserve() {
  return impl_->OutputReserve();
}

ActionRequest StreamCoderManager::Output(utils::Buffer& buffer) {
  return impl_->Output(buffer);
}

utils::Error StreamCoderManager::GetLatestError() const {
  return impl_->GetLatestError();
}

bool StreamCoderManager::forwarding() const { return impl_->forwarding(); }

}  // namespace stream_coder
}  // namespace nekit
