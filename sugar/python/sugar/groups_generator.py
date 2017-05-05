#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import os.path
import sugar.groups

"""
  class Generator hold two directories: top, third party.
  If file in top, pass it to top directory, otherwise to third party.
  Directories class can hold path like 'dir1/dir2/dir3' if have no files
  inside intermediate dir1, dir2 directories. Directories can be splitted,
  merged, ...
  Generator.get_result return generator that yields Result class with
  simple struct: group name, file list of this group
"""

class Result:
  """
    Result of generation - group name + list of files in this group
  """
  def __init__(self, group_name):
    self.group_name = group_name
    self.filename = []

  def add_file(self, filename):
    assert(not filename.endswith('\n'))
    self.filename.append(filename)

"""
Directory entry with list of files and dirs.
self.no_split mean that this directory will not be 'merged' with other
"""
class Directory:
  def __init__(self, path, no_split=False, verbose=False):
    """
      self.path can be empty (for third_party group)
    """
    self.path = path # path without '/' at end
    self.no_split = no_split
    self.verbose = verbose
    self.file_list = []
    self.dir_list = []
    if verbose:
      print('init: path {}, no_split {}'.format(path, no_split))

  def common(self, filename):
    """return empty string if no common dirs in path,
    otherwise return common part ending with '/'
    """
    if not self.path:
      return ''
    assert('/' in filename) # absolute path expected
    dirname = os.path.dirname(filename) + '/'
    self_with_slash = self.path + '/'
    common_part = os.path.commonprefix([dirname, self_with_slash])
    slash_index = common_part.rfind('/')
    if slash_index == -1: # not found
      return ''
    if not common_part[0:slash_index]: # slash excluded
      return ''
    common_part = common_part[0:slash_index+1] # with slash at end
    if not self.no_split:
      return common_part
    if common_part == self_with_slash:
      return common_part
    else:
      return ''

  def print_status(self, status, filename):
    if self.verbose:
      print('[dir:', self.path, '] file:' , filename, '-', status)

  def merge_file(self, head_suffix, tail):
    """Merge to existing directories or create new"""
    assert(len(head_suffix) != 0)
    filename = head_suffix + '/' + tail
    self.print_status('try merge', filename)
    for dirs in self.dir_list:
      if dirs.common(filename):
        return dirs.add_file(filename)
      else:
        message = 'not found in ' + dirs.path
        self.print_status(message, filename)
    self.print_status(
        'merge failed, creating new dir ' + head_suffix,
        filename
    )
    self.dir_list.append(Directory(head_suffix, verbose=self.verbose))
    return self.dir_list[-1].add_file(tail)

  def add_file(self, filename):
    self.print_status('add file', filename)
    head, tail = os.path.split(filename)
    if not self.path:
      return self.merge_file(head, tail)
    if not '/' in filename:
      self.print_status('accept', filename)
      return self.file_list.append(filename)
    common_part = self.common(filename)
    assert(len(common_part) != 0)
    assert(common_part[-1] == '/')

    # common_part = [...] '/'
    # head     : [common_part] [head suffix]
    # self.path: [common_part] [self.path suffix]
    head_suffix = head[len(common_part):]
    self_suffix = self.path[len(common_part):]

    message = \
        'common = ' + \
        common_part + \
        ', suffix = ' + \
        head_suffix + \
        ',' + \
        self_suffix

    self.print_status(message, filename)

    if not self_suffix and not head_suffix:
      # head     : [common_part]
      # self.path: [common_part]
      self.print_status('no suffix', filename)
      assert('/' not in tail)
      return self.add_file(tail)
    if not self_suffix:
      # head     : [common_part] [head suffix]
      # self.path: [common_part]
      return self.merge_file(head_suffix, tail)

    assert(len(self_suffix) != 0)
    # head     : [common_part] ???
    # self.path: [common_part] [self.path suffix]
    self.print_status('merge', filename)
    assert(not self.no_split)

    new_subdir = Directory(self_suffix, verbose=self.verbose)
    new_subdir.dir_list = self.dir_list
    new_subdir.file_list = self.file_list
    self.dir_list = [new_subdir]
    self.file_list = []
    self.path = common_part[0:-1] # without '/'
    return self.add_file(filename)

  def get_result(self, group_name, skip, path=""):
    if path:
      path = path + '/'
    if self.no_split or not self.path:
      group_name = group_name
    else:
      group_name = group_name + r'\\' + self.path
    full_path = path + self.path

    if self.file_list:
      groups = [
          sugar.groups.TmplGroup(group_name),
          sugar.groups.FwdGroup(group_name),
          sugar.groups.SugarGroup(group_name),
          sugar.groups.CMakeGroup(group_name),
      ]

      # this is last group !
      if skip:
        # skip unknown files
        groups.append(sugar.groups.RegularGroup(group_name))
      else:
        # process all
        groups.append(sugar.groups.AnyGroup(group_name))

      for x in list(set(self.file_list)): # remove duplicates
        assert(not x.endswith('\n'))
        file_full_path = full_path + '/' + x # cmake join style, not os.join!
        for group in groups:
          if group.check(x):
            group.add(file_full_path)
            break

      for group in groups:
        if group.result.filename:
          yield group.result

    for dir in sorted(self.dir_list, key=lambda x: x.path):
      for result in dir.get_result(group_name, skip, full_path):
        assert(result.filename)
        yield result

"""
Usage:
* create generator
* add files to generator
* get result (see Result class)
"""
class Generator:
  """
    Hold two main top level directories:
    1) self.top - top directory of project, with path 'top'
    2) self.third_party - fake diretory with no path, hold other directories
  """
  def __init__(self, top, verbose):
    self.top = Directory(top, no_split=True, verbose=verbose)
    self.third_party = Directory('', no_split=True, verbose=verbose)
    self.verbose = verbose
    if self.verbose:
      print("init with top:", top)

  def add(self, source):
    assert(not source.endswith('\n'))
    if self.verbose:
      print("add:", source)
    if self.top.common(source):
      self.top.add_file(source)
    else:
      self.third_party.add_file(source)

  def get_result(self, top_name, third_party_name, skip):
    for x in self.top.get_result(top_name, skip):
      yield x
    for x in self.third_party.get_result(third_party_name, skip):
      yield x
