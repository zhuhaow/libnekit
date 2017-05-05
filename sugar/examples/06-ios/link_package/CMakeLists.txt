# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Link universal library using 'find_package' command

cmake_minimum_required(VERSION 2.8.11)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")
include(sugar_include)

set(
    CMAKE_INSTALL_PREFIX
    "${SUGAR_ROOT}/examples/06-ios/_universal_library/install"
    CACHE
    PATH
    ""
)

project(06-example-ios.link_package CXX)

### Emulate toolchain
set(CMAKE_OSX_SYSROOT "iphoneos")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_DEBUG_POSTFIX d)
### -- end

sugar_include("./sources") # SOURCES

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

# 1. Use CMAKE_INSTALL_PREFIX to find FooConfig.cmake
# 2. Import target Foo::foo from FooConfig.cmake (from FooTargets*.cmake)
find_package(Foo CONFIG REQUIRED)

add_executable(example ${SOURCES})

target_link_libraries(example Foo::foo)

set_target_properties(
    example
    PROPERTIES
    MACOSX_BUNDLE YES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.sugar.ios.LinkPackage"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
