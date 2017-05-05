#!/usr/bin/env python3

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

file_header = """# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)
include(sugar_fatal_error)
include(sugar_status_debug)

function(sugar_generate_warning_flag_by_name warning_flags warning_name)
  sugar_expected_number_of_arguments(${ARGC} 2)

  sugar_status_debug("Flags by name: ${warning_name}")

  ### Check preconditions
  if(is_clang OR is_msvc OR is_gcc)
    # Supported compilers
  else()
    sugar_fatal_error("")
  endif()

  string(COMPARE EQUAL "ALL" "${warning_name}" is_all)
  if(is_all)
    # Skip this (already processed)
    set(${warning_flags} "" PARENT_SCOPE)
    return()
  endif()

  set(result "")

"""

file_footer = """  message("Unknown warning name: ${warning_name}")
  message("List of known warnings: https://github.com/ruslo/leathers/wiki/List")
  sugar_fatal_error("")
endfunction()
"""

all_attrs_header = """# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)

function(sugar_get_all_xcode_warning_attrs attr_list_name)
  sugar_expected_number_of_arguments(${ARGC} 1)

  if(NOT XCODE_VERSION)
    set(${attr_list_name} "" PARENT_SCOPE)
    return()
  endif()

  set(${attr_list_name} "")

"""

all_attrs_footer = """
  set(${attr_list_name} ${${attr_list_name}} PARENT_SCOPE)
endfunction()
"""

attr_by_name_header = """# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)

function(sugar_generate_warning_xcode_attr_by_name warn_flag warn_name)
  sugar_expected_number_of_arguments(${ARGC} 2)

  if(NOT XCODE_VERSION)
    set(${warn_flag} "" PARENT_SCOPE)
    return()
  endif()

"""

attr_by_name_footer = """  set(${warn_flag} "" PARENT_SCOPE)
endfunction()
"""

groups_header = """# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

include(sugar_add_this_to_sourcelist)
sugar_add_this_to_sourcelist()

include(sugar_expected_number_of_arguments)

function(sugar_warning_unpack_one var_name)
  sugar_expected_number_of_arguments(${ARGC} 1)
"""

def generate(main_warnings_table):
  generate_warn_by_name(main_warnings_table)
  generate_all_xcode_warn(main_warnings_table)
  generate_xcode_attr_by_name(main_warnings_table)
  generate_groups_unpacker(main_warnings_table)

def generate_warn_by_name(main_warnings_table):
  cmake_file = open("sugar_generate_warning_flag_by_name.cmake", "w")
  cmake_file.write(file_header)
  for entry in main_warnings_table:
    name = entry.warning_name
    cmake_file.write("  ### {}\n".format(name))
    cmake_file.write(
        """  string(COMPARE EQUAL "{}" "${}" hit)\n""".format(
            name, "{warning_name}"
        )
    )
    cmake_file.write("  if(hit)\n")

    if entry.clang.valid():
      text = entry.clang.cxx_entry(name)
      cmake_file.write("    if(is_clang)\n")
      cmake_file.write("      list(APPEND result \"{}\")\n".format(text))
      cmake_file.write("    endif()\n")

    if entry.gcc.valid():
      text = entry.gcc.cxx_entry(name)
      cmake_file.write("    if(is_gcc)\n")
      cmake_file.write("      list(APPEND result \"{}\")\n".format(text))
      cmake_file.write("    endif()\n")

    if entry.msvc.valid():
      text = entry.msvc.cxx_entry(name)
      cmake_file.write("    if(is_msvc)\n")
      cmake_file.write("      list(APPEND result \"{}\")\n".format(text))
      cmake_file.write("    endif()\n")

    """footer"""
    cmake_file.write("    set(${warning_flags} \"${result}\" PARENT_SCOPE)\n")
    cmake_file.write("    return()\n")
    cmake_file.write("  endif()\n\n")
  cmake_file.write(file_footer)

def generate_all_xcode_warn(main_warnings_table):
  cmake_file = open("sugar_get_all_xcode_warning_attrs.cmake", "w")
  cmake_file.write(all_attrs_header)
  for entry in main_warnings_table:
    if entry.xcode.valid():
      cmake_file.write("  list(APPEND ${attr_list_name} ")
      cmake_file.write("XCODE_ATTRIBUTE_{}".format(entry.xcode.option))
      cmake_file.write(")\n")
  cmake_file.write(all_attrs_footer)

def generate_xcode_attr_by_name(main_warnings_table):
  cmake_file = open("sugar_generate_warning_xcode_attr_by_name.cmake", "w")
  cmake_file.write(attr_by_name_header)
  for entry in main_warnings_table:
    if entry.xcode.valid():
      cmake_file.write("  string(COMPARE EQUAL \"${warn_name}\" \"")
      cmake_file.write(entry.warning_name)
      cmake_file.write("\" hit)\n")
      cmake_file.write("  if(hit)\n")
      cmake_file.write("    set(${warn_flag} \"")
      cmake_file.write("XCODE_ATTRIBUTE_{}".format(entry.xcode.option))
      cmake_file.write("\" PARENT_SCOPE)\n")
      cmake_file.write("    return()\n")
      cmake_file.write("  endif()\n\n")
  cmake_file.write(attr_by_name_footer)

def generate_groups_unpacker(main_warnings_table):
  groups = dict()
  for entry in main_warnings_table:
    if entry.group == "":
      continue
    if entry.group in groups:
      groups[entry.group].append(entry.warning_name)
    else:
      groups[entry.group] = [entry.warning_name]

  cmake_file = open("sugar_warning_unpack_one.cmake", "w")
  cmake_file.write(groups_header)
  for group_name in groups:
    warn_list = ";".join(groups[group_name])
    cmake_file.write("\n  #{}\n".format(group_name))
    cmake_file.write("  string(COMPARE EQUAL \"${${var_name}}\" \"")
    cmake_file.write(group_name)
    cmake_file.write("\" hit)\n")
    cmake_file.write("  if(hit)\n")
    cmake_file.write("    set(${var_name} \"")
    cmake_file.write(warn_list)
    cmake_file.write("\" PARENT_SCOPE)\n")
    cmake_file.write("    return()\n")
    cmake_file.write("  endif()\n")
  cmake_file.write("endfunction()\n")
