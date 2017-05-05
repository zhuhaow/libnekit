#!/usr/bin/python

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import os
import sys

linux = False
ubuntu = False
gentoo = False
cygwin = False
macosx = False
windows = False

platform = sys.platform

if platform in 'win32':
  windows = True
elif platform in 'cygwin':
  cygwin = True
elif platform.startswith('linux'):
  # ubuntu 'linux'
  # gentoo 'linux2'
  linux = True

  sysname, nodename, release, version, machine = os.uname()
  if 'Ubuntu' in version:
    ubuntu = True
  elif 'gentoo' in release:
    gentoo = True
elif platform in 'darwin':
  macosx = True
else:
  sys.exit("python can't detect platform (= ", platform, ")")
