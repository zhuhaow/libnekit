#!/usr/bin/env python3

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

"""
* Wiki table for `leathers` C++ project

Expected format:

### Main table

 Name                        | Clang    | GCC      | MSVC |
-----------------------------|----------|----------|------|
 static-ctor-not-thread-safe | *no*     | *no*     | 4640 |
 switch                      | **same** | **same** | 4062 |
 switch-enum                 | **same** | **same** | 4061 |

### Xcode/Clang table

 Clang                 | Xcode                          | Objective-C |
-----------------------|--------------------------------|-------------|
 bool-conversion       | CLANG_WARN_BOOL_CONVERSION     | no          |
 c++11-extensions      | CLANG_WARN_CXX0X_EXTENSIONS    | no          |
 strict-selector-match | GCC_WARN_STRICT_SELECTOR_MATCH | yes         |
 undeclared-selector   | GCC_WARN_UNDECLARED_SELECTOR   | yes         |

"""

def generate(main_warnings_table):
  groups = set()
  for i in main_warnings_table:
    if i.group != "":
      groups.add(i.group)

  wiki_file = open("wiki-table.txt", "w")

  generate_main_table(main_warnings_table, wiki_file)
  for group in groups:
    generate_group_table(main_warnings_table, wiki_file, group)
  generate_xcode_table(main_warnings_table, wiki_file)

def generate_main_table(main_warnings_table, wiki_file):
  head_name = "Name"
  head_clang = "Clang"
  head_gcc = "GCC"
  head_msvc = "MSVC"

  def calc_max(head, visitor):
    max_len = len(head)
    for x in main_warnings_table:
      cur_len = visitor(x)
      if cur_len > max_len:
        max_len = cur_len
    return max_len + 2

  def name_visitor(table_entry):
    if table_entry.group != "":
      return 0
    return len(table_entry.warning_name)

  def clang_visitor(table_entry):
    if table_entry.group != "":
      return 0
    return len(table_entry.clang.wiki_entry(table_entry.warning_name))

  def gcc_visitor(table_entry):
    if table_entry.group != "":
      return 0
    return len(table_entry.gcc.wiki_entry(table_entry.warning_name))

  def msvc_visitor(table_entry):
    if table_entry.group != "":
      return 0
    return len(table_entry.msvc.wiki_entry(table_entry.warning_name))

  max_name = calc_max(head_name, name_visitor)
  max_clang = calc_max(head_clang, clang_visitor)
  max_gcc = calc_max(head_gcc, gcc_visitor)
  max_msvc = calc_max(head_msvc, msvc_visitor)

  def fill_string(name, max_name):
    result = " " + name + " ";
    assert(max_name >= len(result))
    left = max_name - len(result)
    return result + " " * left

  wiki_file.write("### Main table\n\n")

  s = "{}|{}|{}|{}|\n".format(
      fill_string(head_name, max_name),
      fill_string(head_clang, max_clang),
      fill_string(head_gcc, max_gcc),
      fill_string(head_msvc, max_msvc),
  )
  wiki_file.write(s)

  s = "{}|{}|{}|{}|\n".format(
      '-' * max_name,
      '-' * max_clang,
      '-' * max_gcc,
      '-' * max_msvc,
  )
  wiki_file.write(s)

  for entry in main_warnings_table:
    if entry.group != "":
      continue
    s = "{}|{}|{}|{}|\n".format(
        fill_string(entry.warning_name, max_name),
        fill_string(entry.clang.wiki_entry(entry.warning_name), max_clang),
        fill_string(entry.gcc.wiki_entry(entry.warning_name), max_gcc),
        fill_string(entry.msvc.wiki_entry(entry.warning_name), max_msvc),
    )
    wiki_file.write(s)

def generate_group_table(main_warnings_table, wiki_file, group):
  head_name = "Name"
  head_clang = "Clang"
  head_gcc = "GCC"
  head_msvc = "MSVC"

  def calc_max(head, visitor):
    max_len = len(head)
    for x in main_warnings_table:
      cur_len = visitor(x)
      if cur_len > max_len:
        max_len = cur_len
    return max_len + 2

  def name_visitor(table_entry):
    if table_entry.group != group:
      return 0
    return len(table_entry.warning_name)

  def clang_visitor(table_entry):
    if table_entry.group != group:
      return 0
    return len(table_entry.clang.wiki_entry(table_entry.warning_name))

  def gcc_visitor(table_entry):
    if table_entry.group != group:
      return 0
    return len(table_entry.gcc.wiki_entry(table_entry.warning_name))

  def msvc_visitor(table_entry):
    if table_entry.group != group:
      return 0
    return len(table_entry.msvc.wiki_entry(table_entry.warning_name))

  max_name = calc_max(head_name, name_visitor)
  max_clang = calc_max(head_clang, clang_visitor)
  max_gcc = calc_max(head_gcc, gcc_visitor)
  max_msvc = calc_max(head_msvc, msvc_visitor)

  def fill_string(name, max_name):
    result = " " + name + " ";
    assert(max_name >= len(result))
    left = max_name - len(result)
    return result + " " * left

  wiki_file.write("\n### Table for group: `{}`\n\n".format(group))

  s = "{}|{}|{}|{}|\n".format(
      fill_string(head_name, max_name),
      fill_string(head_clang, max_clang),
      fill_string(head_gcc, max_gcc),
      fill_string(head_msvc, max_msvc),
  )
  wiki_file.write(s)

  s = "{}|{}|{}|{}|\n".format(
      '-' * max_name,
      '-' * max_clang,
      '-' * max_gcc,
      '-' * max_msvc,
  )
  wiki_file.write(s)

  for entry in main_warnings_table:
    if entry.group != group:
      continue
    s = "{}|{}|{}|{}|\n".format(
        fill_string(entry.warning_name, max_name),
        fill_string(entry.clang.wiki_entry(entry.warning_name), max_clang),
        fill_string(entry.gcc.wiki_entry(entry.warning_name), max_gcc),
        fill_string(entry.msvc.wiki_entry(entry.warning_name), max_msvc),
    )
    wiki_file.write(s)

def generate_xcode_table(main_warnings_table, wiki_file):
  head_clang = "Clang"
  head_xcode = "Xcode"
  head_objc = "Objective-C"

  def calc_max(head, visitor):
    max_len = len(head)
    for x in main_warnings_table:
      cur_len = visitor(x)
      if cur_len > max_len:
        max_len = cur_len
    return max_len + 2

  def clang_visitor(table_entry):
    if table_entry.xcode.option == "":
      return 0
    return len(table_entry.clang.option)

  def xcode_visitor(table_entry):
    if table_entry.xcode.option == "":
      return 0
    return len(table_entry.xcode.option)

  def objc_visitor(table_entry):
    if table_entry.xcode.option == "":
      return 0
    if table_entry.objc:
      return 3 # "yes"
    else:
      return 2 # "no"

  max_clang = calc_max(head_clang, clang_visitor)
  max_xcode = calc_max(head_xcode, xcode_visitor)
  max_objc = calc_max(head_objc, objc_visitor)

  def fill_string(name, max_name):
    result = " " + name + " ";
    assert(max_name >= len(result))
    left = max_name - len(result)
    return result + " " * left

  wiki_file.write("\n\n### Xcode/Clang table\n\n")

  s = "{}|{}|{}|\n".format(
      fill_string(head_clang, max_clang),
      fill_string(head_xcode, max_xcode),
      fill_string(head_objc, max_objc),
  )
  wiki_file.write(s)

  s = "{}|{}|{}|\n".format(
      '-' * max_clang,
      '-' * max_xcode,
      '-' * max_objc,
  )
  wiki_file.write(s)

  done_list = []
  for entry in main_warnings_table:
    if entry.xcode.option == "":
      continue
    if entry.clang.option in done_list:
      continue

    done_list.append(entry.clang.option)

    if entry.objc:
      objc = "yes"
    else:
      objc = "no"

    s = "{}|{}|{}|\n".format(
        fill_string(entry.clang.option, max_clang),
        fill_string(entry.xcode.option, max_xcode),
        fill_string(objc, max_objc),
    )
    wiki_file.write(s)
