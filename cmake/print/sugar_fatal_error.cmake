# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

# Warning:
#     do not include/use 'sugar_improper_number_of_arguments',
#     because it use 'sugar_fatal_error' function

function(sugar_fatal_error)
  if(${ARGC} EQUAL 0)
    message(FATAL_ERROR "unexpected number of arguments")
  endif()
  foreach(print_message ${ARGV})
    message("")
    message(${print_message})
    message("")
  endforeach()
  message(FATAL_ERROR "")
endfunction()
