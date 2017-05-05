#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Description of generated groups

import sugar.groups_generator

"""Universal group for files that is not chosen other groups"""
class AnyGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(
        r'{}'.format(group_name)
    )

  def check(self, filename):
    return True

  def add(self, fullpath):
    self.result.add_file(fullpath)

"""Sugar library files"""
class SugarGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(
        r'{}\\[sugar]'.format(group_name)
    )

  def check(self, filename):
    return filename == 'sugar.cmake'

  def add(self, fullpath):
    self.result.add_file(fullpath)

"""Group for .tpp C++ files (template instantiations)"""
class TmplGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(
        r'{}\\[tmpl]'.format(group_name)
    )

  def check(self, filename):
    return filename.endswith('.tpp')

  def add(self, fullpath):
    self.result.add_file(fullpath)

"""Group for .fpp C++ files (forward declared)"""
class FwdGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(r'{}\\[fwd]'.format(group_name))

  def check(self, filename):
    return filename.endswith('.fpp')

  def add(self, fullpath):
    self.result.add_file(fullpath)

"""CMake files"""
class CMakeGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(
        r'{}\\[cmake]'.format(group_name)
    )

  def check(self, filename):
    return (filename == 'CMakeLists.txt') or (filename == 'xcode.environment')

  def add(self, fullpath):
    self.result.add_file(fullpath)

"""Regular C++ files"""
class RegularGroup:
  def __init__(self, group_name):
    self.result = sugar.groups_generator.Result(r'{}'.format(group_name))

  def check(self, filename):
    extensions_list = [
        '.rh', # Windows MSVC
        '.h',
        '.hpp',
        '.ipp',
        '.cpp',
        '.cc',
        '.cxx',
        '.cmake',
        '.inl'
    ]
    for extension in extensions_list:
      if filename.endswith(extension):
        return True
    return False

  def add(self, fullpath):
    self.result.add_file(fullpath)
