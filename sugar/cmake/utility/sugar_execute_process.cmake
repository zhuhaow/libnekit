# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_fatal_error)
include(sugar_improper_number_of_arguments)

# internal variables:
#   _sugar_cmd # command to execute
#   _sugar_execute_result # exit code
#   _sugar_execute_error # error output
macro(sugar_execute_process output)
  sugar_improper_number_of_arguments(${ARGC} 0)
  sugar_improper_number_of_arguments(${ARGC} 1)

  set(_sugar_cmd ${ARGV})
  list(REMOVE_AT _sugar_cmd 0) # remove 'output' variable

  execute_process(
      COMMAND
      ${_sugar_cmd}
      WORKING_DIRECTORY
      "${PROJECT_BINARY_DIR}"
      RESULT_VARIABLE
      _sugar_execute_result
      OUTPUT_VARIABLE
      ${output} # this will modify parent scope variable
      ERROR_VARIABLE
      _sugar_execute_error
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (NOT ${_sugar_execute_result} EQUAL 0)
    set(_sugar_cmd_one_line "")
    foreach(x ${_sugar_cmd})
      set(_sugar_cmd_one_line "${_sugar_cmd_one_line} ${x}")
    endforeach()
    sugar_fatal_error(
        "exit code: ${_sugar_execute_result};"
        "output: ${${output}};"
        "error: ${_sugar_execute_error};"
        "Failed to run command: `${_sugar_cmd_one_line}`"
        " in directory '${PROJECT_BINARY_DIR}'"
    )
  endif()
endmacro()
