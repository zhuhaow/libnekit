// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include "module_b/B.hpp"

#include <iostream> // std::cout

namespace module_b {

void B::hello() {
  std::cout << "Hello from module B" << std::endl;
}

} // namespace module_b
