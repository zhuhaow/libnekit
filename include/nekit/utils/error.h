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
#include <ostream>

#include <boost/any.hpp>
#include <boost/assert.hpp>

#define NE_DEFINE_STATIC_ERROR_CATEGORY(CATEGORY) \
  static CATEGORY& Global##CATEGORY() {           \
    static CATEGORY category;                     \
    return category;                              \
  }

#define NE_DEFINE_NEW_ERROR_CODE(TYPE, NAMESPACE1, NAMESPACE2)        \
  namespace nekit {                                                   \
  namespace utils {                                                   \
  template <>                                                         \
  struct IsErrorCode<::NAMESPACE1::NAMESPACE2::TYPE##ErrorCode>       \
      : public std::true_type {};                                     \
  }                                                                   \
  }                                                                   \
  namespace NAMESPACE1 {                                              \
  namespace NAMESPACE2 {                                              \
  inline ::nekit::utils::Error MakeErrorCode(TYPE##ErrorCode ec) {    \
    return ::nekit::utils::Error(                                     \
        TYPE##ErrorCategory::Global##TYPE##ErrorCategory(), (int)ec); \
  }                                                                   \
  }                                                                   \
  }

namespace nekit {
namespace utils {

class Error;

template <typename EC>
struct IsErrorCode : public std::false_type {};

template <typename EC, typename = std::enable_if_t<IsErrorCode<EC>::value>>
Error MakeErrorCode(EC);

class ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(ErrorCategory)

  virtual std::string Description(const Error& error) const {
    (void)error;
    return "no error";
  };

  virtual std::string DebugDescription(const Error& error) const {
    (void)error;
    return "no error";
  };

  ErrorCategory() = default;
};

class Error final {
 public:
  Error() : Error(ErrorCategory::GlobalErrorCategory(), 0) {}

  Error(const ErrorCategory& category, int error_code)
      : category_{&category}, error_code_{error_code} {};

  template <typename EC, typename = std::enable_if_t<IsErrorCode<EC>::value>>
  Error(EC error_code) {
    *this = MakeErrorCode(error_code);
  }

  explicit operator bool() const { return error_code_; }

  utils::Error Dup() const {
    utils::Error error{*category_, error_code_};

    if (info_) {
      error.info_ = std::make_unique<std::map<int, boost::any>>(*info_);
    }

    return error;
  }

  void CreateInfoDict() {
    info_ = std::make_unique<std::map<int, boost::any>>();
  }

  template <typename T>
  void AddInfo(int key, T value) {
    BOOST_ASSERT(info_);
    info_->emplace(key, value);
  }

  bool HasInfo(int key) const {
    BOOST_ASSERT(info_);
    return info_ && info_->find(key) != info_->cend();
  }

  template <typename T>
  const T& GetInfo(int key) const {
    BOOST_ASSERT(info_);
    return boost::any_cast<const T&>(info_->at(key));
  }

  std::string Description() const { return category_->Description(*this); };

  std::string DebugDescription() const {
    return category_->DebugDescription(*this);
  };

  const ErrorCategory& Category() const { return *category_; }

  int ErrorCode() const { return error_code_; }

 private:
  std::unique_ptr<std::map<int, boost::any>> info_;
  const ErrorCategory* category_;

  int error_code_;
};

inline bool operator==(const Error& err1, const Error& err2) {
  return &err1.Category() == &err2.Category() &&
         err1.ErrorCode() == err2.ErrorCode();
}

inline std::ostream& operator<<(std::ostream& oss, const utils::Error& error) {
  oss << error.Description();
  return oss;
}
}  // namespace utils
}  // namespace nekit
