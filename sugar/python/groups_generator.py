#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import argparse
import os
import sugar.groups_generator
import sys
import textwrap

"""Create generator and add files from inputfile"""
def fill_generator(top, inputfile, verbose):
  top = top.rstrip('/')
  generator = sugar.groups_generator.Generator(top, verbose)
  with open(inputfile, 'r') as file_id:
    for filelist in file_id:
      for filepath in filelist.split(';'):
        generator.add(filepath.rstrip('\n'))
  return generator

def write_verbose(file_id, content, verbose):
  if verbose:
    print(content)
  file_id.write(content)

"""Convert generator entries to cmake file"""
def produce_cmake_file(
    generator,
    top,
    outputfile,
    groupname,
    varname,
    sourcevar,
    skip,
    verbose
):
  with open(outputfile, 'w') as file_id:
    write_verbose(
        file_id,
        '### this file generated automatically '
        'by command: {}\n'.format(' '.join(sys.argv)) +
        'include(sugar_add_this_to_sourcelist)\n'
        'sugar_add_this_to_sourcelist()\n\n',
        verbose
    )
    if varname:
      write_verbose(
          file_id,
          'include(sugar_test_variable_not_empty)\n'
          'sugar_test_variable_not_empty({})\n\n'.format(varname),
          verbose
      )

    all_sources = []
    for group in generator.get_result(groupname, '[third party]', skip):
      files = []
      for filename in group.filename:
        cmake_source_string = (
            '    {}'.format(filename)
        )
        if varname:
          relpath = os.path.relpath(filename, top)
          if not relpath.startswith(r'../'):
            cmake_source_string = (
                '    "${' +
                '{}'.format(varname) +
                '}/' +
                relpath +
                '"'
            )
        files.append(cmake_source_string)
        all_sources.append(cmake_source_string)

      if not files:
        continue

      cmake_chunk = (
          'source_group(\n'
          '    "{}"\n'.format(group.group_name) +
          '    FILES\n' +
          '\n'.join(files) +
          '\n)\n\n'
      )
      write_verbose(file_id, cmake_chunk, verbose)
    if sourcevar and all_sources:
      write_verbose(
          file_id,
          'set({}\n'.format(sourcevar) +
          '\n'.join(all_sources) +
          '\n)\n',
          verbose
      )

def main():
  parser = argparse.ArgumentParser(
      formatter_class = argparse.RawDescriptionHelpFormatter,
      description=textwrap.dedent(r"""
          Generate cmake code that create groups;
          Result will looks like this:
            source_group(
                "[third party]\\[boost]\\mpl\\include/boost/mpl/set"
                FILES
                "${BOOST_ROOT}/mpl/include/boost/mpl/set/set0.hpp"
                "${BOOST_ROOT}/mpl/include/boost/mpl/set/set10.hpp"
            )

          If --top-variable is given additional code that set
          <variable>_SOURCES will be added
          """
      )
  )
  parser.add_argument(
      '--top',
      type=str,
      required=True,
      help='top directory of sources'
  )
  parser.add_argument(
      '--input',
      type=argparse.FileType('r'),
      required=True,
      help='input file with files path'
  )
  parser.add_argument(
      '--output',
      type=argparse.FileType('w'),
      required=True,
      help='output result file (cmake code)'
  )
  parser.add_argument(
      '--top-group-name',
      type=str,
      required=True,
      dest='groupname',
      help = (
          'group name of top (ex. '
          r'[third party]\\[my-lib-1])'
      )
  )
  parser.add_argument(
      '--top-variable',
      type=str,
      required=False,
      default="",
      dest='varname',
      help = (
          'variable for top directory '
          '(ex. BOOST_ROOT, PROJECT_SOURCE_DIR)'
      )
  )
  parser.add_argument(
      '--sourcevar',
      type=str,
      required=False,
      default="",
      help = (
          'source variable, if given set this variable to source list'
          '(ex. GTEST_SOURCES)'
      )
  )
  parser.add_argument(
    '--skip-non-source',
    action='store_true',
    dest='skip',
    required=False,
    default=False,
    help = (
        'skip files with unknown non-source extensions (like .txt, .html,'
        ' .png, ...) (useful when scan directories with docs, pictures, ...)'
    )
  )
  parser.add_argument(
    '--verbose',
    action='store_true',
    dest='verbose',
    help='print a lot info'
  )
  args = parser.parse_args()
  generator = fill_generator(
      args.top,
      args.input.name,
      args.verbose
  )

  produce_cmake_file(
      generator,
      args.top,
      args.output.name,
      args.groupname,
      args.varname,
      args.sourcevar,
      args.skip,
      args.verbose
  )

if __name__ == "__main__":
  main()
