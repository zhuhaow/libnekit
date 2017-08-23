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

#include <string>

// The include order is critical.
#include <boost/asio.hpp>

#include <maxminddb.h>

#include "country_iso_code.h"

namespace nekit {
namespace utils {

class MaxmindLookupResult {
 public:
  MaxmindLookupResult(MMDB_lookup_result_s result);
  MaxmindLookupResult(int error);

  bool error() const;
  bool found() const;

  CountryIsoCode country_iso_code();

 private:
  MMDB_lookup_result_s result_;
  int mmdb_error_{MMDB_SUCCESS};
};

class Maxmind {
 public:
  static bool Initalize(std::string db_file);

  static MaxmindLookupResult Lookup(const std::string &ip);
  static MaxmindLookupResult Lookup(const boost::asio::ip::address &ip);

  template <typename Protocol>
  static MaxmindLookupResult Lookup(
      const boost::asio::ip::basic_endpoint<Protocol> &ip);

 private:
  Maxmind();
  static MMDB_s &GetMmdb();

  static std::string db_file_;
};

}  // namespace utils
}  // namespace nekit
