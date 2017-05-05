# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_status_print)

macro(sugar_find_python3)
  sugar_expected_number_of_arguments(${ARGC} 0)
  if(PYTHONINTERP_FOUND AND NOT ${PYTHON_VERSION_MAJOR} EQUAL 3)
    sugar_status_print("Force update python")
    set(PYTHON_EXECUTABLE "XXX-NOTFOUND")
  endif()

  find_package(PythonInterp 3.2 REQUIRED)
endmacro()
