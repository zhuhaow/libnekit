# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Example of using sugar_add_ios_gtest command to run gtest with ios simulator

cmake_minimum_required(VERSION 2.8.11)
project(03-example-ios-gtest CXX)

### Emulate toolchain
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_OSX_SYSROOT "iphoneos")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.sugar.iOSGTest")
### -- end

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

include("$ENV{SUGAR_ROOT}/cmake/Sugar")
include(sugar_add_ios_gtest)

# Install gtest ios using this project:
#     * 'https://github.com/hunter-packages/gtest'
# Note that it is just example, you don't need to install gtest manually
# if you're using "hunter" package manager (github.com/ruslo/hunter)
set(GTest_DIR "${SUGAR_ROOT}/examples/third_party/_install/ios/lib/cmake/GTest")
find_package(GTest CONFIG REQUIRED)

add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test GTest::main)

enable_testing()
sugar_add_ios_gtest(SimpleTestName simple_test)
