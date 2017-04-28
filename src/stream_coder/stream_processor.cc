#include <nekit/deps/easylogging++.h>
#include <nekit/stream_coder/detail/stream_coder_manager.h>
#include <nekit/stream_coder/stream_processor.h>

namespace nekit {
namespace stream_coder {

template <class T>
StreamProcessor<T>::StreamProcessor() {
  impl_ = new T();
}

template <class T>
StreamProcessor<T>::~StreamProcessor() {
  delete impl_;
}

template <class T>
ActionRequest StreamProcessor<T>::Negotiate() {
  return impl_->Negotiate();
}

template <class T>
BufferReserveSize StreamProcessor<T>::InputReserve() {
  return impl_->InputReserve();
}

template <class T>
ActionRequest StreamProcessor<T>::Input(utils::Buffer& buffer) {
  return impl_->Input(buffer);
}

template <class T>
BufferReserveSize StreamProcessor<T>::OutputReserve() {
  return impl_->OutputReserve();
}

template <class T>
ActionRequest StreamProcessor<T>::Output(utils::Buffer& buffer) {
  return impl_->Output(buffer);
}

template <class T>
utils::Error StreamProcessor<T>::GetLatestError() const {
  return impl_->GetLatestError();
}

template <class T>
bool StreamProcessor<T>::forwarding() const {
  return impl_->forwarding();
}

template <class T>
T* StreamProcessor<T>::implemention() {
  return impl_;
}

}  // namespace stream_coder
}  // namespace nekit
