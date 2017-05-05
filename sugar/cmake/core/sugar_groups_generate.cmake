# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_execute_process)
include(sugar_find_python3)
include(sugar_improper_number_of_arguments)
include(sugar_status_debug)
include(sugar_status_print)
include(sugar_test_variable_not_empty)

function(sugar_groups_generate)
  sugar_improper_number_of_arguments(${ARGC} 0)

  if(NOT MSVC AND NOT XCODE_VERSION)
    sugar_status_print("generate groups skipped (not msvc and not xcode)")
    return()
  endif()

  set(sources ${ARGV})
  list(REMOVE_DUPLICATES sources)
  list(LENGTH sources number_of_files)

  sugar_status_print("Generating groups for ${number_of_files} file(s)")
  sugar_find_python3()

  sugar_test_variable_not_empty(SUGAR_ROOT)
  sugar_test_variable_not_empty(PYTHON_EXECUTABLE)

  set(input_file "${PROJECT_BINARY_DIR}/__sugar_groups_generator.input")
  set(output_file "${PROJECT_BINARY_DIR}/__sugar_groups_generator.output")

  sugar_status_debug("Run python script")
  file(
      WRITE
      ${input_file}
      "${sources}"
  )

  set(cmd "")
  list(APPEND cmd "${PYTHON_EXECUTABLE}")
  list(APPEND cmd "${SUGAR_ROOT}/python/groups_generator.py")
  list(APPEND cmd --top)
  list(APPEND cmd "${CMAKE_CURRENT_LIST_DIR}")
  list(APPEND cmd --input)
  list(APPEND cmd "${input_file}")
  list(APPEND cmd --output)
  list(APPEND cmd "${output_file}")
  list(APPEND cmd --top-group-name)
  list(APPEND cmd "[top]")

  sugar_execute_process(temp ${cmd})
  include(${output_file})
endfunction()
