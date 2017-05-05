#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import datetime
import os
import sys
import tempfile
import shutil

import detail.os_detect
import detail.os
import detail.print

class Error(Exception):
  def __init__(self, message):
    self.message = message

class NotExist(Error):
  pass

def get_trash_root_dir(filename):
  """Generate root directory for trash directory for given filename"""
  file_partition = detail.os.get_partition(filename)
  if detail.os_detect.windows:
    return file_partition

  home = os.environ['HOME']
  home_partition = detail.os.get_partition(home)
  if home_partition != file_partition:
    print(
         'Warning: {} (in {} partition) not in HOME partition ({})'
         ', operation may be slow'.format(
             detail.print.file(filename),
             file_partition,
             home_partition
         )
    )
  return home

def get_trash_path(filename):
  """Generate trash directory pathname for given filename"""
  return os.path.join(get_trash_root_dir(filename), '.trash-data')

def check_trash_path(filename):
  """Check trash directory exist for given filename, create it otherwise"""
  trash_path = get_trash_path(filename)
  if not os.path.exists(trash_path):
    os.makedirs(trash_path)

# Raise NotExist
def trash(objname, ignore_not_exist=False):
  objname = objname.rstrip(os.sep)
  if not os.path.exists(objname):
    if ignore_not_exist:
      return
    raise NotExist(detail.print.not_exist(objname))

  trash_path = get_trash_path(objname)
  today = datetime.date.today()
  year_dir = today.strftime("%Y")
  month_dir = today.strftime("%m-%B")
  day_dir = today.strftime("%d-%A")
  current_trash_dir = os.path.join(trash_path, year_dir, month_dir, day_dir)
  if not os.path.exists(current_trash_dir):
    os.makedirs(current_trash_dir)

  src = objname
  obj_prefix = os.path.split(objname)[-1] + '-'
  handle, dst = tempfile.mkstemp(
      prefix=obj_prefix,
      dir=current_trash_dir
  )
  detail.print.from_to_start('trash', src, dst)
  os.remove(dst) # remove temp
  shutil.move(src, dst) # don't use os.rename (Error 18: Cross-device link)
  detail.print.from_to_stop()
