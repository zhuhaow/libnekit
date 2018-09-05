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

#include <regex>

#include "error.h"

#define NE_REGEX_ERROR_INFO_KEY 1

namespace nekit {
namespace utils {
enum class RegexErrorCode { GeneralError = 1 };

class RegexErrorCategory : public ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(RegexErrorCategory);

  static utils::Error FromRegexError(const std::regex_error& error) {
    // We can just save the `error_type` of the `regex_error` as error code in
    // `Error`, but the standard does not require it can be represented
    // as an int.
    utils::Error err(RegexErrorCategory::GlobalRegexErrorCategory(),
                     (int)RegexErrorCode::GeneralError);
    err.CreateInfoDict();
    err.AddInfo(NE_REGEX_ERROR_INFO_KEY, error.code());
    return err;
  }

  std::string Description(const Error& error) const override {
    return std::regex_error(error.GetInfo<std::regex_constants::error_type>(
                                NE_REGEX_ERROR_INFO_KEY))
        .what();
  }

  std::string DebugDescription(const Error& error) const override {
    return Description(error);
  }
};

}  // namespace utils
}  // namespace nekit

NE_DEFINE_NEW_ERROR_CODE(Regex, nekit, utils)
