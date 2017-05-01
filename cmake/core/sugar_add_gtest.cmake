# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_add_ios_gtest)
include(sugar_status_debug)

function(sugar_add_gtest)
  string(COMPARE EQUAL "${CMAKE_OSX_SYSROOT}" "iphoneos" is_ios)
  if(is_ios)
    sugar_status_debug("Use sugar_add_ios_gtest")
    sugar_status_debug("ARGV: [${ARGV}]")
    sugar_add_ios_gtest(${ARGV})
  else()
    sugar_status_debug("Use cmake add_test")
    add_test(${ARGV})
  endif()
endfunction()
