# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Example of linking to timestamp library with current build time

cmake_minimum_required(VERSION 2.8.11)
project(11-example-timestamp CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")
include(sugar_link_timestamp)

add_executable(timestamp_bin "main.cpp")
sugar_link_timestamp(timestamp_bin)
