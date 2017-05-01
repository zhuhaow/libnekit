# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

# Detecting _WIN32_WINNT value for windows host
# Based on: http://stackoverflow.com/a/17845462/2288008

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_get_WIN32_WINNT result)
  sugar_expected_number_of_arguments(${ARGC} 1)
  if(NOT WIN32)
    sugar_fatal_error("Function is only available for windows")
  endif()

  if(NOT CMAKE_SYSTEM_VERSION)
    sugar_fatal_error("CMAKE_SYSTEM_VERSION is empty")
  endif()

  set(ver "${CMAKE_SYSTEM_VERSION}")
  string(REPLACE "." "" ver "${ver}")
  string(REGEX REPLACE "([0-9])" "0\\1" ver "${ver}")

  set(${result} "0x${ver}" PARENT_SCOPE)
endfunction()
