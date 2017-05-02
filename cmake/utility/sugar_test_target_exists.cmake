# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_test_target_exists target)
  sugar_expected_number_of_arguments(${ARGC} 1)
  if (NOT TARGET ${target})
    sugar_fatal_error("Target '${target}' not exists")
  endif()
endfunction()
