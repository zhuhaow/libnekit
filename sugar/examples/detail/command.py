#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import subprocess
import sys

import detail.os_detect
import detail.utility

def print_install_memo_and_exit(cmd):
  print(cmd, 'command not found, please install it')
  if detail.os_detect.windows:
    pass
  elif detail.os_detect.cygwin:
    pass
  elif detail.os_detect.ubuntu: # check it before linux
    print("You can try 'sudo apt-get install {}'".format(cmd))
  elif detail.os_detect.gentoo: # check it before linux
    print("You can try 'emerge -v {}'".format(cmd))
  elif detail.os_detect.linux:
    pass
  elif detail.os_detect.macosx:
    print("You can try 'sudo port install {}'".format(cmd))
  else:
    detail.utility.unreachable()
  sys.exit("Error")

def test_exist(cmd):
  """Run |which| and if |cmd| not found return |False|, otherwise |True|"""
  if get_absolute_path(cmd) == '':
    return False
  else:
    return True

def get_absolute_path(cmd):
  """Run |which| to get absolute path to command or return empty string"""
  if detail.os_detect.windows:
    which = 'where'
  else:
    which = 'which'
  try:
    subprocess.check_output([which, which])
  except subprocess.CalledProcessError:
    sys.exit("Internal error: '{}' not found".format(which))
  try:
    output = subprocess.check_output([which, cmd], universal_newlines=True)
    output_list = output.split('\n')[:-1] # remove last
    if len(output_list) != 1:
      sys.exit("Unexpected {} result".format(which))
    return output_list[0]
  except subprocess.CalledProcessError:
    return ''

# TODO, 3.3 version use shutil.which
def check_exist(cmd):
  """Run |which| and if |cmd| not found print help installation text and exit"""
  if not test_exist(cmd):
    print_install_memo_and_exit(cmd)

def run(cmd):
  """Run given |cmd|, check exit OK and return list of output lines"""
  command_name = cmd[0]
  check_exist(command_name)
  try:
    output = subprocess.check_output(cmd, universal_newlines=True)
    return output.split('\n')[:-1] # remove last
  except subprocess.CalledProcessError as exc:
    print('Failed to run "{}"'.format(' '.join(cmd)))
    raise exc
