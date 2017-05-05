// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "A.hpp"

#include <cstdlib>
#include <exception>

#include "B.hpp"

void A::foo() {
#if (A_TARGET_MACRO) == 1
  if(false) {
    std::abort();
  }
#endif
#if (A_DIRECTORY_MACRO) == 1
  if(false) {
    std::terminate();
  }
#endif
  boo();
}

void A::boo() {
  B b;
#if (A_DEBUG) == 1
  b.hacky_method(13);
#elif (A_NDEBUG) == 1
  b.foo(42, 15);
#else
# error "Something wrong"
#endif
}
