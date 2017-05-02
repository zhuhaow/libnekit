# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_status_debug)
include(sugar_test_file_exists)

macro(sugar_include dir)
  sugar_expected_number_of_arguments(${ARGC} 1)

  if(IS_ABSOLUTE "${dir}/sugar.cmake")
    set(_include_file "${dir}/sugar.cmake")
  else()
    set(_include_file "${CMAKE_CURRENT_LIST_DIR}/${dir}/sugar.cmake")
  endif()

  sugar_status_debug(
      "including '${_include_file}' from ${CMAKE_CURRENT_LIST_DIR}"
  )

  if(SUGAR_STATUS_DEBUG)
    sugar_test_file_exists("${_include_file}")
  endif()

  include("${_include_file}")
endmacro()
