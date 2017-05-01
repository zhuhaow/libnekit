# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_execute_process)
include(sugar_expected_number_of_arguments)
include(sugar_find_python3)
include(sugar_test_target_exists)
include(sugar_test_variable_not_empty)

function(sugar_link_timestamp targetname)
  sugar_expected_number_of_arguments(${ARGC} 1)
  sugar_test_target_exists(${targetname})

  set(timestamp_directory "${PROJECT_BINARY_DIR}/_timestamp")
  set(timestamp_header "${timestamp_directory}/timestamp.hpp")
  set(timestamp_source "${timestamp_directory}/timestamp.cpp")

  file(MAKE_DIRECTORY "${timestamp_directory}")

  target_include_directories(${targetname} PRIVATE "${timestamp_directory}")

  sugar_find_python3()

  sugar_test_variable_not_empty(PYTHON_EXECUTABLE)
  sugar_test_variable_not_empty(SUGAR_ROOT)

  set(
      update_cmd
      "${PYTHON_EXECUTABLE}"
      "${SUGAR_ROOT}/python/timestamp_update.py"
      "--header"
      "${timestamp_header}"
      "--source"
      "${timestamp_source}"
  )

  # init files
  sugar_execute_process(${update_cmd})

  if(NOT TARGET _timestamp)
    add_library(_timestamp "${timestamp_header}" "${timestamp_source}")

    # add update command
    add_custom_target(
        _timestamp_update
        ALL
        COMMAND
        ${update_cmd}
        COMMENT
        "Creating timestamp"
    )

    add_dependencies(_timestamp _timestamp_update)
  endif()

  target_link_libraries(${targetname} _timestamp)
endfunction()
