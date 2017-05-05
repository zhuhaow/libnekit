# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

if(DEFINED MODULE_B_SUBDIR_SUGAR_CMAKE)
  return()
else()
  set(MODULE_B_SUBDIR_SUGAR_CMAKE 1)
endif()

include(sugar_files)

sugar_files(
    MODULE_A_SOURCES
    Boo.hpp
    Boo.cpp
)
