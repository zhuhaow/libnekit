#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import os.path
import re

import detail.command
import detail.os_detect

def empty_dir(directory):
  """Check directory is empty"""
  if not os.path.isdir(directory):
    return False
  return len(os.listdir(directory)) == 0

def get_partition(path):
  """Get partition of a file or directory"""
  if not os.path.exists(path):
    sys.exit(path, 'not exist')
  if detail.os_detect.windows:
    # get first letters before ':'
    return os.path.abspath(path).upper().split(sep=':')[0]
  else:
    output = detail.command.run(['df', path])
    # output looks like this:
    # Filesystem     1K-blocks    Used Available Use% Mounted on
    # /dev/sda3       38267376 9219432  27097380  26% /
    assert(len(output) == 2)
    header = output[0]
    info = output[1]
    last_index = re.match('^Filesystem[ ]*', header).end()
    # /dev/sda3       382...
    #                 ^^^^^^ (1)
    #          ^^^^^^^ (2)
    # (1) strip last digits that belongs to '1K-blocks' section
    # (2) strip trailing spaces
    return info[:last_index].rstrip('[0123456789]').rstrip()

def remove(object):
  if not os.path.exists(object):
    return
  if os.path.isfile(object):
    os.remove(object)
  if os.path.isdir(object):
    os.removedirs(object)

def first_is_subdirectory_of_second(dir1, dir2):
  dir1 = dir1.rstrip(os.sep)
  dir2 = dir2.rstrip(os.sep)
  if dir1 == dir2:
    return True
  if not dir1.startswith(dir2):
    return False
  rest = dir1[len(dir2):]
  if rest.startswith(os.sep):
    return True
  return False

def win_to_cygwin(winpath):
  """run `cygpath winpath` to get cygwin path"""
  x = detail.command.run(['cygpath', winpath])
  assert(len(x) == 1)
  return x[0]

def cygwin_to_win(cygwinpath):
  """run `cygpath --absolute --windows` to get windows path"""
  x = detail.command.run(['cygpath', '--absolute', '--windows', cygwinpath])
  assert(len(x) == 1)
  return x[0]
