#!/usr/bin/python

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import sys
import traceback

def unreachable():
  """Use this to mark dead code"""
  traceback.print_stack()
  sys.exit('Internal error: unreachable')

def remove_duplicates(some_list):
  return list(set(some_list))
