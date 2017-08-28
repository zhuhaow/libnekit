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

#include <cstddef>
#include <cstdint>

#include <CxxUrl/url.h>
#include <picohttpparser/picohttpparser.h>

#include "../config.h"

namespace nekit {
namespace utils {
class HttpHeaderParser {
 public:
  enum { ParseError = -1, HeaderIncomplete = -2 };

  int Parse(const uint8_t* data, const size_t data_len);

  std::string method() const;
  std::string path() const;
  std::string relative_path() const;

  std::string host() const;
  uint16_t port() const;
  int minor_version() const;

  struct phr_header* header_fields();
  size_t header_field_size() const;

 private:
  bool ParseUrl();

  const char *method_, *path_;
  size_t method_len_, path_len_;
  int minor_version_;
  struct phr_header headers_[NEKIT_HTTP_HEADER_MAX_FIELD];
  size_t headers_count_{NEKIT_HTTP_HEADER_MAX_FIELD};

  uint16_t port_;

  Url url_;
};
}  // namespace utils
}  // namespace nekit
