# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# http://www.kitware.com/blog/home/post/390

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)

function(sugar_echo_target_property target property)
  sugar_expected_number_of_arguments(${ARGC} 2)
  if(NOT TARGET ${target})
    sugar_fatal_error("Not a target: ${target}")
  endif()

  # v for value, d for defined, s for set
  get_property(v TARGET ${target} PROPERTY ${property})
  get_property(d TARGET ${target} PROPERTY ${property} DEFINED)
  get_property(s TARGET ${target} PROPERTY ${property} SET)

  # only produce output for values that are set
  if(s)
    message("${property}: [${v}] (defined = ${d}, set = ${s})")
  endif()
endfunction()
