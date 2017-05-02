# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_test_file_exists filename)
  sugar_expected_number_of_arguments(${ARGC} 1)

  # http://www.cmake.org/cmake/help/v2.8.10/cmake.html#command:if
  # ... Behavior is well-defined only for full paths ...
  if(NOT IS_ABSOLUTE "${filename}")
    set(filename "${CMAKE_CURRENT_LIST_DIR}/${filename}")
  endif()

  if (NOT EXISTS "${filename}")
    sugar_fatal_error("File '${filename}' not exists;")
  endif()

  if (IS_DIRECTORY "${filename}")
    sugar_fatal_error("'${filename}' is a directory, not a file;")
  endif()
endfunction()
