# Copyright (c) 2015, Tomas Zemaitis
# All rights reserved.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include(clear_env)
include(fatal_error)

# Fix try_compile
set(MACOSX_BUNDLE_GUI_IDENTIFIER com.example)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
set(CMAKE_MACOSX_BUNDLE YES)

set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)
set (CMAKE_C_FLAGS_INIT "")
set (CMAKE_CXX_FLAGS_INIT "-fvisibility=hidden -fvisibility-inlines-hidden")

set(IPHONEOS_ARCHS armv7;arm64)
set(IPHONESIMULATOR_ARCHS i386;x86_64)

set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "System root for iOS" FORCE)
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")

# find 'iphoneos' and 'iphonesimulator' roots and version
find_program(XCODE_SELECT_EXECUTABLE xcode-select)
if(NOT XCODE_SELECT_EXECUTABLE)
  fatal_error("xcode-select not found")
endif()

if(CMAKE_VERSION VERSION_LESS "3.5")
  fatal_error(
      "CMake minimum required version for iOS is 3.5 (current ver: ${CMAKE_VERSION})"
  )
endif()

execute_process(
    COMMAND
    ${XCODE_SELECT_EXECUTABLE}
    "-print-path"
    OUTPUT_VARIABLE
    XCODE_DEVELOPER_ROOT # /.../Xcode.app/Contents/Developer
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_program(XCODEBUILD_EXECUTABLE xcodebuild)
if(NOT XCODEBUILD_EXECUTABLE)
  fatal_error("xcodebuild not found")
endif()

# Check version exists
execute_process(
    COMMAND
    "${XCODEBUILD_EXECUTABLE}" "-showsdks"
    COMMAND
    "grep" "iphoneos"
    COMMAND
    "egrep" "[[:digit:]]+\\.[[:digit:]]+" "-o"
    COMMAND
    "tail" "-1"
    OUTPUT_VARIABLE IOS_SDK_VERSION
    RESULT_VARIABLE IOS_SDK_VERSION_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# iPhone simulator root
set(
    IPHONESIMULATOR_ROOT
    "${XCODE_DEVELOPER_ROOT}/Platforms/iPhoneSimulator.platform/Developer"
)
if(NOT EXISTS "${IPHONESIMULATOR_ROOT}")
  fatal_error(
      "IPHONESIMULATOR_ROOT not found (${IPHONESIMULATOR_ROOT})\n"
      "XCODE_DEVELOPER_ROOT: ${XCODE_DEVELOPER_ROOT}\n"
  )
endif()

# iPhone simulator SDK root
set(
    IPHONESIMULATOR_SDK_ROOT
    "${IPHONESIMULATOR_ROOT}/SDKs/iPhoneSimulator${IOS_SDK_VERSION}.sdk"
)

if(NOT EXISTS ${IPHONESIMULATOR_SDK_ROOT})
  polly_fatal_error(
      "IPHONESIMULATOR_SDK_ROOT not found (${IPHONESIMULATOR_SDK_ROOT})\n"
      "IPHONESIMULATOR_ROOT: ${IPHONESIMULATOR_ROOT}\n"
      "IOS_SDK_VERSION: ${IOS_SDK_VERSION}\n"
  )
endif()

# iPhone root
set(
    IPHONEOS_ROOT
    "${XCODE_DEVELOPER_ROOT}/Platforms/iPhoneOS.platform/Developer"
)
if(NOT EXISTS "${IPHONEOS_ROOT}")
  fatal_error(
      "IPHONEOS_ROOT not found (${IPHONEOS_ROOT})\n"
      "XCODE_DEVELOPER_ROOT: ${XCODE_DEVELOPER_ROOT}\n"
  )
endif()

# iPhone SDK root
set(IPHONEOS_SDK_ROOT "${IPHONEOS_ROOT}/SDKs/iPhoneOS${IOS_SDK_VERSION}.sdk")

if(NOT EXISTS ${IPHONEOS_SDK_ROOT})
  fatal_error(
      "IPHONEOS_SDK_ROOT not found (${IPHONEOS_SDK_ROOT})\n"
      "IPHONEOS_ROOT: ${IPHONEOS_ROOT}\n"
      "IOS_SDK_VERSION: ${IOS_SDK_VERSION}\n"
  )
endif()

string(COMPARE EQUAL "${IOS_DEPLOYMENT_SDK_VERSION}" "" _is_empty)
if(_is_empty)
  set(
      CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET
      "${IOS_SDK_VERSION}"
  )
else()
  set(
      CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET
      "${IOS_DEPLOYMENT_SDK_VERSION}"
  )
endif()

# Emulate OpenCV toolchain --
set(IOS YES)
# -- end

# Set iPhoneOS architectures
set(archs "")
foreach(arch ${IPHONEOS_ARCHS})
  set(archs "${archs} ${arch}")
endforeach()
set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] "${archs}")
set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] "${archs}")

# Set iPhoneSimulator architectures
set(archs "")
foreach(arch ${IPHONESIMULATOR_ARCHS})
  set(archs "${archs} ${arch}")
endforeach()
set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] "${archs}")
set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphonesimulator*] "${archs}")

# Introduced in iOS 9.0
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE YES)

# This will set CMAKE_CROSSCOMPILING to TRUE.
# CMAKE_CROSSCOMPILING needed for try_run:
# * https://cmake.org/cmake/help/latest/command/try_run.html#behavior-when-cross-compiling
# (used in CURL)
set(CMAKE_SYSTEM_NAME "Darwin")

