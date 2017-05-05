# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

if(DEFINED SOURCES_MODULE_A_SUGAR_CMAKE)
  return()
else()
  set(SOURCES_MODULE_A_SUGAR_CMAKE 1)
endif()

include(sugar_files)
include(sugar_include)

sugar_include("./subdir")

sugar_files(
  MODULE_A_SOURCES
  A.hpp
  A.cpp
)
