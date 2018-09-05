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

#include <boost/system/error_code.hpp>

#include "error.h"

#define NE_BOOST_ERROR_INFO_KEY 1

namespace nekit {
namespace utils {
enum class BoostErrorCode { Error = 1 };

class BoostErrorCategory : public ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(BoostErrorCategory)

  static utils::Error FromBoostError(const boost::system::error_code& ec) {
    utils::Error e{BoostErrorCategory::GlobalBoostErrorCategory(),
                   (int)BoostErrorCode::Error};
    e.CreateInfoDict();
    e.AddInfo(NE_BOOST_ERROR_INFO_KEY, ec);
    return e;
  }

  static boost::system::error_code ToBoostError(const utils::Error& error) {
    return error.GetInfo<boost::system::error_code>(NE_BOOST_ERROR_INFO_KEY);
  }

  std::string Description(const utils::Error& error) const override {
    return "boost error: " +
           error.GetInfo<boost::system::error_code>(NE_BOOST_ERROR_INFO_KEY)
               .message();
  }

  std::string DebugDescription(const utils::Error& error) const override {
    return Description(error);
  }
};

}  // namespace utils
}  // namespace nekit

NE_DEFINE_NEW_ERROR_CODE(Boost, nekit, utils)
