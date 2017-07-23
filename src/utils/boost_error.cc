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

#include "nekit/utils/boost_error.h"

// template <typename BoostErrorType,
//           const boost::system::error_category &boost_category>
// struct BoostErrorCategory : public std::error_category {
//   const char *name() const noexcept override { return boost_category.name();
//   }

//   std::string message(int ec) const override {
//     return
//     boost::asio::error::make_error_code(static_cast<BoostErrorType>(ec))
//         .message();
//   }

//   static BoostErrorCategory<BoostErrorType, boost_category> &category() {
//     static BoostErrorCategory<BoostErrorType, boost_category> category;
//     return category;
//   }
// };

// Template is not working since we can't take the error category as template
// argument.
#define NEKIT_BOOST_ERROR_CODE_TYPE(TYPE) \
  static_cast<boost::asio::error::TYPE##_errors>

#define NEKIT_BOOST_ERROR_CATEGORY2(CATEGORY, ERROR_TYPE)            \
  namespace {                                                        \
  struct Boost##CATEGORY : public std::error_category {              \
    const char *name() const noexcept override {                     \
      return boost::asio::error::get_##CATEGORY##_category().name(); \
    }                                                                \
                                                                     \
    std::string message(int ec) const override {                     \
      return boost::asio::error::make_error_code(                    \
                 NEKIT_BOOST_ERROR_CODE_TYPE(ERROR_TYPE)(ec))        \
          .message();                                                \
    }                                                                \
  };                                                                 \
                                                                     \
  const Boost##CATEGORY boost_##CATEGORY##_error_category{};         \
  }

#define NEKIT_BOOST_ERROR_CATEGORY(CATEGORY, ERROR_TYPE) \
  NEKIT_BOOST_ERROR_CATEGORY2(CATEGORY, ERROR_TYPE)

NEKIT_BOOST_ERROR_CATEGORY(system, basic)
NEKIT_BOOST_ERROR_CATEGORY(netdb, netdb)
NEKIT_BOOST_ERROR_CATEGORY(addrinfo, addrinfo)
NEKIT_BOOST_ERROR_CATEGORY(misc, misc)

namespace std {
std::error_code make_error_code(boost::system::error_code ec) {
  if (ec.category() == boost::asio::error::system_category) {
    return {ec.value(), boost_system_error_category};
  } else if (ec.category() == boost::asio::error::netdb_category) {
    return {ec.value(), boost_netdb_error_category};
  } else if (ec.category() == boost::asio::error::addrinfo_category) {
    return {ec.value(), boost_addrinfo_error_category};
  } else if (ec.category() == boost::asio::error::misc_category) {
    return {ec.value(), boost_misc_error_category};
  }
  // The truth is, the boost uses system_category for all asio related errors,
  // others are simply aliases.
  return {ec.value(), boost_system_error_category};
}
}  // namespace std
