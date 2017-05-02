# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_test_directory_exists directory)
  sugar_expected_number_of_arguments(${ARGC} 1)

  # http://www.cmake.org/cmake/help/v2.8.10/cmake.html#command:if
  # ... Behavior is well-defined only for full paths ...
  if(NOT IS_ABSOLUTE "${directory}")
    set(directory "${CMAKE_CURRENT_LIST_DIR}/${directory}")
  endif()

  if (NOT EXISTS "${directory}")
    sugar_fatal_error("Directory '${directory}' not exists;")
  endif()

  if (NOT IS_DIRECTORY "${directory}")
    sugar_fatal_error("'${directory}' is a file, not a directory;")
  endif()
endfunction()
