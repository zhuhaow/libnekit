#!/usr/bin/env python3

# Copyright (c) 2013-2014, Ruslan Baratov
# All rights reserved.

help_wiki = "https://github.com/ruslo/sugar/wiki/Examples-testing"

import os
import argparse
import re
import sys
import subprocess
import copy

import detail.trash
import detail.os_detect
import detail.argparse

top_dir = os.getcwd()

parser = argparse.ArgumentParser()

parser.add_argument(
    '--include',
    type=detail.argparse.is_dir,
    nargs='*',
    help="include this directory patterns (low priority)"
)

parser.add_argument(
    '--exclude',
    type=detail.argparse.is_dir,
    nargs='*',
    help="exclude this directory patterns (high priority)"
)

parser.add_argument(
    '--libcxx',
    action='store_true',
    help='compile and link with libcxx library'
)

parser.add_argument(
    '--sim',
    action='store_true',
    help='build for ios simulator (Xcode only)'
)

args = parser.parse_args()

if args.exclude:
  print('exclude directories: {}'.format(args.exclude))

if args.include:
  print('include directories: {}'.format(args.include))

# test:
#  Unix Makefiles _builds/make-debug
#  Xcode _builds/xcode
#  Visual Studio 11 _builds/msvc

class Config:
  def __init__(self, generator, generator_params, directory, build):
    self.generator = generator
    self.generator_params = generator_params
    self.directory = directory
    self.build = build

  def info(self, build_dir):
    info = '[{}] [{}'.format(build_dir, self.generator)
    if self.generator_params:
      info += ' + {}'.format(self.generator_params)
    info += '] [{}]'.format(self.build)
    return info

configs = []

if detail.os_detect.windows:
  sys.exit("Not tested (see {})".format(help_wiki))
  configs.append(Config('Visual Studio', '', 'msvc', 'nmake')) # ???
else:
  if args.libcxx:
    stdlib_flag = "'-stdlib=libc++'"
    libcxx_flag = "-DCMAKE_CXX_FLAGS={}".format(stdlib_flag)
  else:
    libcxx_flag = ''
  debug_opt = '-DCMAKE_BUILD_TYPE=Debug {}'.format(libcxx_flag)
  release_opt = '-DCMAKE_BUILD_TYPE=Release {}'.format(libcxx_flag)
  default_opt = '{}'.format(libcxx_flag)

  configs.append(Config('Unix Makefiles', default_opt, 'make-default', 'make'))
  configs.append(Config(
      'Unix Makefiles', release_opt, 'make-release', 'make'
  ))
  configs.append(Config(
      'Unix Makefiles', debug_opt, 'make-debug', 'make'
  ))

if detail.os_detect.macosx:
  if args.libcxx:
    params = " -DCMAKE_EXE_LINKER_FLAGS='-stdlib=libc++' {}".format(libcxx_flag)
  else:
    params = ''
  configs.append(Config('Xcode', params, 'xcode', 'xcodebuild'))

gtest_version = '1.7.0-hunter-6'
gtest_result = 'gtest-' + gtest_version
gtest_tar_gz = 'v{}.tar.gz'.format(gtest_version)
gtest_src = 'https://github.com/hunter-packages/gtest/archive/' + gtest_tar_gz

os.chdir('third_party')
if not os.path.exists(gtest_tar_gz):
  subprocess.check_call(['wget', gtest_src])
  subprocess.check_call(['tar', '-xf', gtest_tar_gz])
  os.chdir(gtest_result)

  install_prefix = os.path.join(top_dir, 'third_party', '_install', 'native')
  install_prefix = '-DCMAKE_INSTALL_PREFIX={}'.format(install_prefix)
  if args.libcxx:
    lib_flags = '-DCMAKE_CXX_FLAGS=-stdlib=libc++'
  else:
    lib_flags = ''
  subprocess.check_call(
      ['cmake', install_prefix, lib_flags, '-H.', '-B_builds/native']
  )
  subprocess.check_call(
      ['cmake', '--build', '_builds/native', '--target', 'install', '--config', 'Release']
  )

  if detail.os_detect.macosx:
    toolchain = '-DCMAKE_TOOLCHAIN_FILE={}/cmake/iOS.cmake'.format(os.getcwd())
    install_prefix = os.path.join(top_dir, 'third_party', '_install', 'ios')
    install_prefix = '-DCMAKE_INSTALL_PREFIX={}'.format(install_prefix)
    subprocess.check_call(
        ['cmake', '-H.', '-B_builds/ios', '-GXcode', toolchain, install_prefix]
    )
    subprocess.check_call(
        ['cmake', '--build', '_builds/ios', '--target', 'install', '--config', 'Release']
    )
    subprocess.check_call(
        ['cmake', '--build', '_builds/ios', '--target', 'install', '--config', 'Debug']
    )

os.chdir(top_dir)

done_list = []

def run_cmake_test(root, config_in):
  config = copy.deepcopy(config_in)

  library_install = False
  if re.match('./06-ios/_universal_library', root):
    library_install = True

  if config.generator == 'Xcode':
    if re.match('./00-detect', root):
      config.generator_params = '' # remove warning

  # skip Xcode specific
  if re.match('./07-cocoa-application', root) and config.generator != 'Xcode':
    print("{}: skip (Xcode only)".format(config.generator))
    return

  check_simulator = False
  if re.match('./03-ios-gtest', root):
    check_simulator = True
    if config.generator != 'Xcode':
      print("{}: skip (Xcode only)".format(config.generator))
      return

  if re.match('./04-gtest-universal', root):
    check_simulator = True

  if re.match('./06-ios', root):
    check_simulator = True
    if config.generator != 'Xcode':
      print("{}: skip (Xcode only)".format(config.generator))
      return

  if check_simulator and config.generator == 'Xcode':
    if args.sim:
      build_sdk = 'iphonesimulator -arch i386'
    else:
      build_sdk = 'iphoneos'
    config.build += ' -sdk {}'.format(build_sdk)

  build_dir=os.path.join(root, '_builds', config.directory)
  detail.trash.trash(build_dir, ignore_not_exist=True)

  os.makedirs(build_dir)
  os.chdir(build_dir)

  config_info = config.info(build_dir)
  try:
    print('##### {}'.format(config_info))

    command = ['cmake', '-G', '{}'.format(config.generator)]
    command += config.generator_params.split()
    if library_install:
      command.append(
          '-DCMAKE_INSTALL_PREFIX={}/../../install'.format(os.getcwd())
      )
    command.append('../..')
    subprocess.check_call(command)
    print('build...')
    if config.generator == 'Xcode':
      build_release = '{} -configuration Release'.format(config.build)
      build_debug = '{} -configuration Debug'.format(config.build)
      subprocess.check_call(build_release.split())
      subprocess.check_call(build_debug.split())
    else:
      subprocess.check_call(config.build.split())

    if library_install:
      # additional install step
      subprocess.check_call(
          ['xcodebuild', '-target', 'install', '-configuration', 'Release']
      )
      subprocess.check_call(
          ['xcodebuild', '-target', 'install', '-configuration', 'Debug']
      )
    print('done')
  except subprocess.CalledProcessError:
    sys.exit('run failed in "{}" directory'.format(root))

  done_list.append(config_info)
  os.chdir(top_dir)

  # check library installed (xcodebuild may exit 0 even if build failed)
  if library_install:
    install_base = os.path.join(root, 'install', 'lib')
    lib1 = os.path.join(install_base, 'libfoo.a')
    if not os.path.exists(lib1):
      sys.exit("{} not found".format(lib1))
    lib2 = os.path.join(install_base, 'libfood.a')
    if not os.path.exists(lib2):
      sys.exit("{} not found".format(lib2))

def hit_regex(root, pattern_list):
  if not pattern_list:
    return False
  for pattern_entry in pattern_list:
    if pattern_entry and re.match(pattern_entry, root):
      return True
  return False

for root, dirs, files in os.walk('./'):
  for filename in files:
    if filename != 'CMakeLists.txt':
      continue
    if hit_regex(root, args.exclude):
      print("skip (exclude list): '{}'".format(root))
      continue
    if args.include and not hit_regex(root, args.include):
      print("skip (not in include list): '{}'".format(root))
      continue
    if re.search(r'/{}'.format(gtest_result), root):
      print("skip service temporary project: {}".format(root))
      continue
    file_path = os.path.join(root, filename)
    print('check file = {}'.format(file_path))
    file_id = open(file_path)
    content = file_id.read()
    if not re.search(r'\nproject(.*)\n', content):
      continue
    detail.trash.trash(os.path.join(root, 'install'), ignore_not_exist=True)
    for config in configs:
      run_cmake_test(root, config)

print('DONE LIST:')
for x in done_list:
  print(x)
