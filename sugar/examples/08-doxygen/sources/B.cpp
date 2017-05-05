// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "B.hpp"

#include "A.hpp"

double B::foo(int x, int y) {
#if defined(A_TARGET_MACRO)
  A a;
  a.foo();
#endif
  return 3.14159 + x + y;
}

double B::hacky_method(int x) {
  return 7.0 + (x - x);
}
