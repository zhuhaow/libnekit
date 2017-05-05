# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_test_variable_not_empty variable_name)
  sugar_expected_number_of_arguments(${ARGC} 1)
  string(COMPARE EQUAL "${${variable_name}}" "" is_empty)
  if(is_empty)
    sugar_fatal_error("Please, set '${variable_name}' variable")
  endif()
endfunction()
