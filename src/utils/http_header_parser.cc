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

#include "nekit/utils/http_header_parser.h"

#include <cstdlib>
#include <regex>

#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace nekit {
namespace utils {
int HttpHeaderParser::Parse(const uint8_t *data, const size_t data_len) {
  int result = phr_parse_request(reinterpret_cast<const char *>(data), data_len,
                                 &method_, &method_len_, &path_, &path_len_,
                                 &minor_version_, headers_, &headers_count_, 0);

  if (result > 0) {
    if (boost::iequals(std::string(method_, method_len_), "connect")) {
      // Turn the request path into a valid URL. The scheme is not used for CONNECT anyway.
      url_ = "https://" + path();
    } else {
      url_ = path();
    }
    if (ParseUrl()) {
      return result;
    } else {
      return ParseError;
    }
  }

  return result;
}

std::string HttpHeaderParser::method() const {
  return std::string(method_, method_len_);
}

std::string HttpHeaderParser::path() const {
  return std::string(path_, path_len_);
}

std::string HttpHeaderParser::relative_path() const {
  std::regex path_regex("http.?:\\/\\/.*?(\\/.*)", std::regex::ECMAScript |
                                                       std::regex::icase |
                                                       std::regex::optimize);
  std::smatch match_result;
  std::string full_path =
      path();  // `regex_match` does not permit temporary string.
  BOOST_VERIFY(std::regex_match(full_path, match_result, path_regex));
  return match_result[1].str();
}

std::string HttpHeaderParser::host() const { return url_.host(); }

uint16_t HttpHeaderParser::port() const { return port_; }

int HttpHeaderParser::minor_version() const { return minor_version_; }

struct phr_header *HttpHeaderParser::header_fields() {
  return headers_;
}

size_t HttpHeaderParser::header_field_size() const { return headers_count_; }

bool HttpHeaderParser::ParseUrl() {
  try {
    url_.ip_version();
  } catch (...) {
    return false;
  }

  try {
    port_ = boost::lexical_cast<uint16_t>(url_.port());
  } catch (...) {
    if (url_.port() != "") {
      return false;
    }

    if (boost::iequals(url_.scheme(), "https")) {
      port_ = 443;
    } else {
      port_ = 80;
    }
    return true;
  }

  return true;
}

}  // namespace utils
}  // namespace nekit
