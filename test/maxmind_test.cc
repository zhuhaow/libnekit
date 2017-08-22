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

#include <gtest/gtest.h>

#include "nekit/utils/maxmind.h"

using namespace nekit::utils;

class Environment : public ::testing::Environment {
 public:
  void SetUp() { assert(Maxmind::Initalize("GeoLite2-Country.mmdb")); }
};

::testing::Environment* const env =
    ::testing::AddGlobalTestEnvironment(new Environment);

TEST(MaxmindUnitTest, CountryIsoCodeLookup) {
  MaxmindLookupResult result = Maxmind::Lookup("127.0.0.1");
  ASSERT_FALSE(result.error());
  ASSERT_FALSE(result.found());
  ASSERT_EQ(result.country_iso_code(), CountryIsoCode::XX);

  result = Maxmind::Lookup("8.8.8.8");
  ASSERT_FALSE(result.error());
  ASSERT_TRUE(result.found());
  ASSERT_EQ(result.country_iso_code(), CountryIsoCode::US);
}
