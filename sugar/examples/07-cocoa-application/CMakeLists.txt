# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Cocoa project example.
# Create project as Xcode: OS X -> Application -> Cocoa Application

cmake_minimum_required(VERSION 2.8.11)
project(07-cocoa-application.example CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")

include(sugar_fatal_error)
include(sugar_include)

include_directories("${SUGAR_ROOT}/examples/third_party")

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

sugar_include("./sources") # fill SOURCES
sugar_include("./resources") # fill RESOURCES

add_executable(cocoa-test-application ${SOURCES} ${RESOURCES})

set_target_properties(
    cocoa-test-application
    PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
        "${SUGAR_ROOT}/examples/plist/macosx/default.plist.in"
    RESOURCE "${RESOURCES}"
    MACOSX_BUNDLE YES
)

target_link_libraries(cocoa-test-application "-framework AppKit")
