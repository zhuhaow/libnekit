// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include <cstdlib> // EXIT_SUCCESS
#include <iostream> // std::cout

#include "module_a/A.hpp"
#include "module_b/B.hpp"

int main() {
  std::cout << "test from B main" << std::endl;
  module_a::A::hello();
  module_b::B::hello();
  return EXIT_SUCCESS;
}
