#!/usr/bin/env python3

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

import os

"""
Generate file for each table entry and some extra files:
* pop
* push
* sugar.cmake
"""

push_text = """// This file generated automatically:
// https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

// Copyright (c) 2014, Ruslan Baratov
// All rights reserved.

#include <boost/predef/compiler.h>

#if defined(LEATHERS_PUSH_)
# error "Not ending 'leathers/push' include detected;"
    "every 'push' must ends with 'pop'; "
    "see `https://github.com/ruslo/leathers/wiki/Pitfalls` for more info"
#else
# define LEATHERS_PUSH_
#endif

#if (BOOST_COMP_CLANG)
# pragma clang diagnostic push
#endif

#if (BOOST_COMP_GNUC) && !(BOOST_COMP_CLANG)
# pragma GCC diagnostic push
#endif

#if (BOOST_COMP_MSVC)
# pragma warning(push)
#endif
"""

pop_text = """// This file generated automatically:
// https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

// Copyright (c) 2014, Ruslan Baratov
// All rights reserved.

#if !defined(LEATHERS_PUSH_)
# error "'leathers/push' include not detected "
    "or already closed with other 'pop'; "
    "see README.txt for more info"
#endif

#undef LEATHERS_PUSH_

#if (BOOST_COMP_CLANG)
# pragma clang diagnostic pop
#endif

#if (BOOST_COMP_GNUC) && !(BOOST_COMP_CLANG)
# pragma GCC diagnostic pop
#endif

#if (BOOST_COMP_MSVC)
# pragma warning(pop)
#endif

"""

sugar_text = """# This file generated automatically:
# https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if(DEFINED LEATHERS_SOURCE_LEATHERS_SUGAR_CMAKE_)
  return()
else()
  set(LEATHERS_SOURCE_LEATHERS_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(
    LEATHERS_SOURCES
    ###
    push
    pop
    ###
    all
    ###
"""

header_text = """// This file generated automatically:
// https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression

// Copyright (c) 2014, Ruslan Baratov
// All rights reserved.
"""

header_check_push = """
#if !defined(LEATHERS_PUSH_)
# error "`leathers/{}` used "
      "without `leathers/push`, "
      "see README.txt for more info"
#endif
"""

header_check_self = """
#if defined(LEATHERS_{}_)
# error "`leathers/{}` "
    "already included; see README.txt for more info"
#else
# define LEATHERS_{}_
#endif
"""

header_clang = """
#if (BOOST_COMP_CLANG)
# if __has_warning("-W{}")
#  pragma clang diagnostic ignored "-W{}"
# endif
#endif
"""

header_gcc = """
#if (BOOST_COMP_GNUC) && !(BOOST_COMP_CLANG)
# pragma GCC diagnostic ignored "-W{}"
#endif
"""

header_msvc = """
#if (BOOST_COMP_MSVC)
# pragma warning(disable: {})
#endif
"""

def to_macro(name):
  return name.upper().replace('-', '_').replace('+', 'X')

def generate(main_warnings_table):
  if not os.path.exists("leathers"):
    os.mkdir("leathers")
  generate_push()
  generate_pop(main_warnings_table)
  generate_all(main_warnings_table)
  generate_sugar_cmake(main_warnings_table)
  for x in main_warnings_table:
    generate_header(x)

  groups = set()
  for x in main_warnings_table:
    if x.group != "":
      groups.add(x.group)
  for group in groups:
    assert(len(group) != 0)
    generate_group(group, main_warnings_table)

def generate_push():
  push_file = open(os.path.join("leathers", "push"), "w")
  push_file.write(push_text)

def generate_pop(main_warnings_table):
  pop_file = open(os.path.join("leathers", "pop"), "w")
  pop_file.write(pop_text)
  for entry in main_warnings_table:
    text = to_macro(entry.warning_name)
    pop_file.write("#undef LEATHERS_{}_\n".format(text))

def generate_all(main_warnings_table):
  all_file = open(os.path.join("leathers", "all"), "w")
  all_file.write(header_text)
  all_file.write('\n')
  for entry in main_warnings_table:
    all_file.write("#include <leathers/{}>\n".format(entry.warning_name))

def generate_sugar_cmake(main_warnings_table):
  sugar_file = open(os.path.join("leathers", "sugar.cmake"), "w")
  sugar_file.write(sugar_text)
  for entry in main_warnings_table:
    sugar_file.write("    {}\n".format(entry.warning_name))
  sugar_file.write(")\n")

def generate_header(table_entry):
  name = table_entry.warning_name
  macro = to_macro(name)
  header_file = open(os.path.join("leathers", name), "w")
  header_file.write(header_text)
  header_file.write(header_check_push.format(name))
  header_file.write(header_check_self.format(macro, name, macro))
  if table_entry.clang.valid():
    x = table_entry.clang.cxx_entry(name)
    header_file.write(header_clang.format(x, x))
  if table_entry.gcc.valid():
    x = table_entry.gcc.cxx_entry(name)
    header_file.write(header_gcc.format(x, x))
  if table_entry.msvc.valid():
    header_file.write(header_msvc.format(table_entry.msvc.cxx_entry(name)))

def generate_group(group, main_warning_table):
  assert(len(group) != 0)
  group_file = open(os.path.join("leathers", group), "w")
  group_file.write(header_text)
  group_file.write("\n")
  for x in main_warning_table:
    if x.group == group:
      group_file.write("#include <leathers/{}>\n".format(x.warning_name))
