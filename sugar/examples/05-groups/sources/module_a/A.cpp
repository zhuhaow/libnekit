// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "module_a/A.hpp"

#include <iostream> // std::cout

namespace module_a {

void A::hello() {
  std::cout << "Hello from module A" << std::endl;
}

} // namespace module_a
