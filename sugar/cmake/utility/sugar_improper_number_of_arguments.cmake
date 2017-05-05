# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_improper_number_of_arguments given improper_number)
  sugar_expected_number_of_arguments(${ARGC} 2)
  if(${given} EQUAL ${improper_number})
    sugar_fatal_error(
        "Improper number of arguments given (= ${given})."
    )
  endif()
endfunction()
