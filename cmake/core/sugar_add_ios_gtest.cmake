# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_fatal_error)
include(sugar_find_python3)
include(sugar_improper_number_of_arguments)
include(sugar_status_print)
include(sugar_test_target_exists)
include(sugar_test_variable_not_empty)

function(sugar_add_ios_gtest testname targetname)
  sugar_improper_number_of_arguments(${ARGC} 0)
  sugar_improper_number_of_arguments(${ARGC} 1)

  set(test_argv ${ARGV})
  list(REMOVE_AT test_argv 0) # remove 'testname' variable
  list(REMOVE_AT test_argv 0) # remove 'targetname' variable

  sugar_test_target_exists(${targetname})

  find_program(IOS_SIM "ios-sim" HINTS ${IOS_SIM_ROOT})
  if(NOT IOS_SIM)
    sugar_fatal_error(
        "ios-sim not found, please install it from:"
        "https://github.com/phonegap/ios-sim/releases"
        "and add to PATH"
    )
  endif()
  sugar_status_print("Use ios-sim: ${IOS_SIM}")
  sugar_find_python3()

  sugar_test_variable_not_empty(PYTHON_EXECUTABLE)
  sugar_test_variable_not_empty(SUGAR_ROOT)
  add_test(
      NAME
      ${testname}
      WORKING_DIRECTORY
      "${PROJECT_BINARY_DIR}"
      COMMAND
      "${PYTHON_EXECUTABLE}"
      "${SUGAR_ROOT}/python/ios_simulator_launcher.py"
      "--sim"
      "${IOS_SIM}"
      "--target"
      "${targetname}"
      "--args"
      ${test_argv}
      "--configuration"
      $<CONFIGURATION>
  )
endfunction()
