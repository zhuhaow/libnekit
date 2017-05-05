# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Simple cmake C++ project file example.

cmake_minimum_required(VERSION 2.8.11)
project(01-example-simple CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")

include(sugar_include)
sugar_include("./sources")
   # update all sources variables (load sugar.cmake files)

add_subdirectory("./sources")
   # create targets (load CMakeLists.txt)
