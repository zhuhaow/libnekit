#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import argparse
import os

def is_dir(name):
  if os.path.isdir(name):
    return name.rstrip(os.sep)
  message = name + ' is not a directory'
  raise argparse.ArgumentTypeError(message)

def is_dir_or_file(name):
  if os.path.isfile(name):
    return name
  try:
    return is_dir(name)
  except argparse.ArgumentTypeError:
    message = name + ' is not a file or directory'
    raise argparse.ArgumentTypeError(message)
