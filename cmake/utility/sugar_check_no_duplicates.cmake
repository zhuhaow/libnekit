# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_improper_number_of_arguments)
include(sugar_fatal_error)

function(sugar_check_no_duplicates)
  if(${ARGC} EQUAL 0)
    return()
  endif()

  set(origin_list ${ARGV})
  set(without_duplicates ${ARGV})
  list(REMOVE_DUPLICATES without_duplicates)

  # delete elements one by one, check size reduced by 1
  foreach(element ${without_duplicates})
    list(LENGTH origin_list expected_length)
    math(EXPR expected_length "${expected_length} - 1")
    list(REMOVE_ITEM origin_list ${element})
    list(LENGTH origin_list real_length)
    if(NOT ${real_length} EQUAL ${expected_length})
      sugar_fatal_error("Duplicate detected: ${element}")
    endif()
  endforeach()
endfunction()
