# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

if(DEFINED MODULE_A_SUBDIR_SUGAR_CMAKE)
  return()
else()
  set(MODULE_A_SUBDIR_SUGAR_CMAKE 1)
endif()

include(sugar_files)

sugar_files(
    MODULE_A_SOURCES
    Foo.hpp
    Foo.cpp
)
