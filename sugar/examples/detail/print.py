#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

def object(objname):
  return "'" + objname + "'"

def file(filename):
  return object(filename)

def dir(dirname):
  return object(dirname)

def not_exist(filename):
  return object(filename) + ' not exist'

def dir_not_exist(dirname):
  return 'directory ' + not_exist(dirname)

def file_not_exist(dirname):
  return 'file ' + not_exist(dirname)

def from_to_start(message, from_name, to):
  """Start 'from -> to' move"""
  print(
      message + ': ' + object(from_name) + '\n    -> ' + object(to) + ' ...',
      end=' '
  )

def from_to_stop():
  """Stop 'from -> to' move"""
  print('ok')
