# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_improper_number_of_arguments)

function(sugar_status_print)
  sugar_improper_number_of_arguments(${ARGC} 0)
  foreach(print_message ${ARGV})
    if(SUGAR_STATUS_PRINT OR SUGAR_STATUS_DEBUG)
      message(STATUS "[sugar] ${print_message}")
    endif()
  endforeach()
endfunction()
