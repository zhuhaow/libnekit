# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_check_no_duplicates)
include(sugar_improper_number_of_arguments)
include(sugar_status_debug)

macro(sugar_files result)
  sugar_improper_number_of_arguments(${ARGC} 0)
  sugar_improper_number_of_arguments(${ARGC} 1)

  set(_files ${ARGV})
  list(REMOVE_AT _files 0) # remove 'result'

  set(_abs_files "")
  foreach(_x ${_files})
    if(IS_ABSOLUTE ${_x})
      list(APPEND _abs_files "${_x}")
    else()
      list(APPEND _abs_files "${CMAKE_CURRENT_LIST_DIR}/${_x}")
    endif()
  endforeach()

  sugar_status_debug("check no duplicates")
  if(SUGAR_STATUS_DEBUG)
    sugar_check_no_duplicates(${_abs_files}) # this is slow!
  endif()

  sugar_status_debug(
      "Append to '${result}'"
      "files: ${_abs_files}"
  )

  list(APPEND ${result} ${_abs_files})
endmacro()
