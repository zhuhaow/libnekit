#!/usr/bin/env python3

# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

import argparse
import datetime
import os
import time

parser = argparse.ArgumentParser(
    description = """
        Create two C++ files: timestamp.{hpp,cpp}
        File timestamp.hpp holds declaration of 'timestamp' variable with
        type 'const char*'. timestamp.hpp will not changed if it's already
        exists. File timestamp.cpp holds definition of 'timestamp' variable
        and will be changed every time script run.
    """
)

parser.add_argument(
    '--header',
    type=str,
    required=True,
    help='full path to timestamp.hpp'
)

parser.add_argument(
    '--source',
    type=str,
    required=True,
    help='full path to timestamp.cpp'
)

args = parser.parse_args()

header_name = args.header

if not os.path.exists(header_name):
  header_id = open(header_name, 'w')
  header_id.write(
      '// this file generated automatically by timestamp_update.py script\n'
      '//     https://github.com/ruslo/sugar\n'
      '\n'
      '#ifndef TIMESTAMP_HPP_\n'
      '#define TIMESTAMP_HPP_\n'
      '\n'
      'const char* timestamp();\n'
      '\n'
      '#endif // TIMESTAMP_HPP_\n'
  )

def timestamp_string():
  timestamp = time.time()
  dtime = datetime.datetime.fromtimestamp(timestamp)
  return dtime.strftime('%Y-%m-%d %H:%M:%S')

source_name = args.source

source_id = open(source_name, 'w')
source_id.write(
    '// this file generated automatically by timestamp_update.py script\n'
    '//     https://github.com/ruslo/sugar\n'
    '\n'
    '#include "timestamp.hpp"\n'
    '\n'
    'const char* timestamp() {\n'
    '    return "' + timestamp_string() + '";\n' +
    '}\n'
)
