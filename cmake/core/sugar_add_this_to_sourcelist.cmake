# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Warning:
#     this function is called from every *.cmake file, no includes here (!)
macro(sugar_add_this_to_sourcelist)
  if(NOT ${ARGC} EQUAL 0)
    message(FATAL_ERROR "no arguments expected")
  endif()
  list(APPEND SUGAR_SOURCES "${CMAKE_CURRENT_LIST_FILE}")
endmacro()

sugar_add_this_to_sourcelist() # self add
