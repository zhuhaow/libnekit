// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "Foo.hpp"

#include <iostream> // std::cout

void Foo::print_info() {
#ifdef NDEBUG
  std::cout << "Hello from libfoo.a (release)" << std::endl;
#else
  std::cout << "Hello from libfood.a (debug)" << std::endl;
#endif
}
