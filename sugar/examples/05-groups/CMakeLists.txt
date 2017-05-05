# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Example of generating groups for Xcode and MSVC

cmake_minimum_required(VERSION 2.8.11)
project(05-example-groups CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")

include(sugar_include)

sugar_include("./sources")

include_directories("./sources")

add_subdirectory("./sources/module_a/main")
add_subdirectory("./sources/module_b/main")
