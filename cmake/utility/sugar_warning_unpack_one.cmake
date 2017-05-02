# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)

function(sugar_warning_unpack_one var_name)
  sugar_expected_number_of_arguments(${ARGC} 1)

  #special-members
  string(COMPARE EQUAL "${${var_name}}" "special-members" hit)
  if(hit)
    set(${var_name} "assign-base-inaccessible;assign-could-not-be-generated;copy-ctor-could-not-be-generated;dflt-ctor-base-inaccessible;dflt-ctor-could-not-be-generated;user-ctor-required" PARENT_SCOPE)
    return()
  endif()

  #compatibility-c++98
  string(COMPARE EQUAL "${${var_name}}" "compatibility-c++98" hit)
  if(hit)
    set(${var_name} "c++98-compat;c++98-compat-pedantic" PARENT_SCOPE)
    return()
  endif()

  #inline
  string(COMPARE EQUAL "${${var_name}}" "inline" hit)
  if(hit)
    set(${var_name} "automatic-inline;force-not-inlined;not-inlined;unreferenced-inline" PARENT_SCOPE)
    return()
  endif()
endfunction()
