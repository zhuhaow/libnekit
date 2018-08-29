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

#include <boost/any.hpp>
#include <boost/assert.hpp>

namespace nekit {
namespace utils {

class Error;

class ErrorCategory {
 public:
  static ErrorCategory& GlobalDefaultErrorCategory() {
    static ErrorCategory default_error_category;
    return default_error_category;
  }

  friend class Error;

 protected:
  virtual std::string Description(const Error& error) const {
    (void)error;
    return "no error";
  };

  virtual std::string DebugDescription(const Error& error) const {
    (void)error;
    return "no error";
  };

  ErrorCategory();
};

class Error final {
 public:
  Error() : Error(ErrorCategory::GlobalDefaultErrorCategory(), 0, false) {}

  Error(const ErrorCategory& category, int error_code,
        bool has_error_info = false)
      : category_{&category}, error_code_{error_code} {
    if (has_error_info) {
      info_ = std::make_unique<std::map<int, boost::any>>();
    }
  };

  explicit operator bool() const { return error_code_; }

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
    return boost::any_cast<T>(info_->at(key));
  }

  std::string Description() { return category_->Description(*this); };

  std::string DebugDescription() { return category_->DebugDescription(*this); };

  const ErrorCategory& Category() const { return *category_; }

  int ErrorCode() const { return error_code_; }

 private:
  std::unique_ptr<std::map<int, boost::any>> info_;
  const ErrorCategory* category_;

  int error_code_;
};

bool operator==(const Error& err1, const Error& err2) {
  return &err1.Category() == &err2.Category() &&
         err1.ErrorCode() == err2.ErrorCode();
}

}  // namespace utils
}  // namespace nekit
