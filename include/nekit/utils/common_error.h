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

#pragma once

#include <hedley/hedley.h>

#include "error.h"

namespace nekit {
namespace utils {
enum class CommonErrorCode : int { EndOfFile = 1, UnknownError };

class CommonErrorCategory : public ErrorCategory {
 public:
  static CommonErrorCategory& GlobalCommonErrorCategory() {
    static CommonErrorCategory common_error_category;
    return common_error_category;
  }

  bool IsEof(const Error& error) const {
    return &error.Category() == &GlobalCommonErrorCategory() &&
           (CommonErrorCode)error.ErrorCode() == CommonErrorCode::EndOfFile;
  }

  Error Eof() const {
    return Error(GlobalCommonErrorCategory(), (int)CommonErrorCode::EndOfFile);
  }

  Error UnknownError() const {
    return Error(GlobalCommonErrorCategory(),
                 (int)CommonErrorCode::UnknownError);
  }

 protected:
  std::string Description(const Error& error) const {
    switch ((CommonErrorCode)error.ErrorCode()) {
      case CommonErrorCode::EndOfFile:
        return "end of file";
      case CommonErrorCode::UnknownError:
        return "unknown error";
      default:
        HEDLEY_UNREACHABLE_RETURN("");
    }
  }

  std::string DebugDescription(const Error& error) const {
    return Description(error);
  }
};
}  // namespace utils
}  // namespace nekit
