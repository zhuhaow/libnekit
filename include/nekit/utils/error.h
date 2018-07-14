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

#include <map>
#include <memory>
#include <system_error>

#include <boost/any.hpp>

namespace nekit {
namespace utils {

enum class NEKitErrorCode {
  NoError,
  GeneralError,
  Canceled,
  MemoryAllocationFailed
};
std::error_code make_error_code(NEKitErrorCode);

}  // namespace utils
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::utils::NEKitErrorCode> : true_type {};
}  // namespace std

namespace nekit {
namespace utils {

class Error {
 public:
  Error() : Error(NEKitErrorCode::NoError) {}
  Error(std::error_code ec) : ec_{ec} {}
  template <typename ErrorCodeEnum,
            typename =
                std::enable_if_t<std::is_error_code_enum<ErrorCodeEnum>::value>>
  Error(ErrorCodeEnum ec) : ec_{make_error_code(ec)} {}

  explicit operator bool() const { return bool(ec_); }

  const std::error_code& ErrorCode() const { return ec_; }

  template <typename T>
  void AddInfo(int key, T value) {
    if (!info_) {
      info_ = std::make_shared<std::map<int, boost::any>>();
    }

    info_->emplace(key, value);
  }

  bool HasInfo(int key) const {
    return info_ && info_->find(key) != info_->cend();
  }
  template <typename T>
  const T& GetInfo(int key) const {
    return boost::any_cast<T>(info_->at(key));
  }

 private:
  std::error_code ec_;
  std::shared_ptr<std::map<int, boost::any>> info_;
};

inline std::error_code make_error_code(Error error) {
  return error.ErrorCode();
}
}  // namespace utils
}  // namespace nekit

namespace std {
template <>
struct is_error_code_enum<nekit::utils::Error> : true_type {};
}  // namespace std
