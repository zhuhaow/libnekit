// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "A.hpp"
#include "C.hpp"
#include "Group.hpp"

void some_func();

int main() {
  A a;
  a.foo();

  some_func();
}

void some_func() {
  A a;
  a.boo();
}
