#!/usr/bin/env python3

# Copyright (c) 2013, 2015, Ruslan Baratov
# All rights reserved.

import argparse
import os
import re
import sys

wiki = 'https://github.com/ruslo/sugar/wiki/Collecting-sources'
base = os.path.basename(__file__)

def first_is_subdirectory_of_second(subdir_name, dir_name):
  subdir_name = subdir_name.rstrip(os.sep)
  dir_name = dir_name.rstrip(os.sep)
  if subdir_name == dir_name:
    return True
  if not subdir_name.startswith(dir_name):
    return False
  rest = subdir_name[len(dir_name):]
  if rest.startswith(os.sep):
    return True
  return False

class Generator:
  def __init__(self):
    self.parser = argparse.ArgumentParser(
        description='Generate sugar.cmake files according to directory struct'
    )
    self.exclude_dirs = []

  def parse(self):
    self.parser.add_argument(
        '--top',
        type=str,
        required=True,
        help='top directory of sources'
    )

    self.parser.add_argument(
        '--var',
        type=str,
        required=True,
        help='variable name'
    )

    self.parser.add_argument(
        '--exclude-dirs',
        type=str,
        nargs='*',
        help='Ignore this directories'
    )

    self.parser.add_argument(
        '--exclude-filenames',
        type=str,
        nargs='*',
        help='Ignore this filenames'
    )

  def make_header_guard(dir):
    dir = dir.upper()
    dir = re.sub(r'\W', '_', dir)
    dir = re.sub('_+', '_', dir)
    dir = dir.lstrip('_')
    dir = dir.rstrip('_')
    dir += '_'
    return dir

  def process_file(relative, source_variable, file_id, filelist, dirlist):
    file_id.write(
        '# This file generated automatically by:\n'
        '#   {}\n'
        '# see wiki for more info:\n'
        '#   {}\n\n'.format(base, wiki)
    )
    relative += '/sugar.cmake'
    hg = Generator.make_header_guard(relative)
    file_id.write(
        'if(DEFINED {})\n'
        '  return()\n'
        'else()\n'
        '  set({} 1)\n'
        'endif()\n\n'.format(hg, hg)
    )
    if filelist:
      file_id.write('include(sugar_files)\n')
    if dirlist:
      file_id.write('include(sugar_include)\n')
    if filelist or dirlist:
      file_id.write('\n')

    if dirlist:
      for x in dirlist:
        file_id.write("sugar_include({})\n".format(x))
      file_id.write('\n')

    if filelist:
      file_id.write("sugar_files(\n")
      file_id.write("    {}\n".format(source_variable))
      for x in filelist:
        file_id.write("    {}\n".format(x))
      file_id.write(")\n")

  def is_excluded(self, dir_name):
    for x in self.exclude_dirs:
      if first_is_subdirectory_of_second(dir_name, x):
        return True
    return False

  def create(self):
    args = self.parser.parse_args()

    cwd = os.getcwd()
    for x in args.exclude_dirs:
      x_abs = os.path.abspath(x)
      if not os.path.exists(x_abs):
        sys.exit('Path `{}` not exists'.format(x_abs))
      self.exclude_dirs.append(x_abs)

    if args.exclude_filenames:
      exclude_filenames = args.exclude_filenames
    else:
      exclude_filenames = []
    exclude_filenames += ['sugar.cmake', 'CMakeLists.txt', '.DS_Store']

    source_variable = args.var
    for rootdir, dirlist, filelist in os.walk(args.top):
      for x in exclude_filenames:
        try:
          filelist.remove(x)
        except ValueError:
          pass # ignore if not in list

      rootdir = os.path.abspath(rootdir)

      if self.is_excluded(rootdir):
        continue

      new_dirlist = []
      for x in dirlist:
        x_abs = os.path.join(rootdir, x)
        if not self.is_excluded(x_abs):
          new_dirlist.append(x)

      relative = os.path.relpath(rootdir, cwd)
      with open('{}/sugar.cmake'.format(rootdir), 'w') as file_id:
        Generator.process_file(
            relative, source_variable, file_id, filelist, new_dirlist
        )

  def run():
    generator = Generator()
    generator.parse()
    generator.create()

if __name__ == '__main__':
  Generator.run()
