# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_fatal_error)

function(sugar_expected_number_of_arguments given expected)
  # Do some self check.
  if(NOT ${ARGC} EQUAL 2)
    sugar_fatal_error(
        "Incorrect usage of 'sugar_expected_number_of_arguments', "
        "expected '2' arguments but given '${ARGC}'."
    )
  endif()

  if(NOT ${given} EQUAL ${expected})
    sugar_fatal_error(
        "Incorrect number of arguments '${given}', expected '${expected}'."
    )
  endif()
endfunction()
