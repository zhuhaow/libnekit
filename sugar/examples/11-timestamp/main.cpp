// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#include <stdlib.h> // EXIT_SUCCESS
#include <iostream> // std::cout
#include "timestamp.hpp"

int main() {
  std::cout << "Hello, I was built at " << timestamp() << std::endl;
  return EXIT_SUCCESS;
}
