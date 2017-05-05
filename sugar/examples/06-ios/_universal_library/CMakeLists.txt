# Copyright (c) 2013-2014, Ruslan Baratov
# All rights reserved.

# Example of build and install universal ("fat", iphone + simulator) iOS library

# Note:
#     leading underscore need for test.py build this project first,
#     because link_library example use it

cmake_minimum_required(VERSION 2.8.12)
project(06-example-ios.universal_library CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")
include(sugar_include)

sugar_include("./sources")

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

include(CMakePackageConfigHelpers) # configure_package_config_file

### Emulate toolchain
set(CMAKE_OSX_SYSROOT "iphoneos")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_MACOSX_BUNDLE YES)
### -- end

add_library(foo ${SOURCES})
# https://github.com/forexample/package-example
# Install layout
set(CONFIG_DEST lib/cmake/Foo)
set(LIB_DEST lib)
set(INCL_DEST include)

target_include_directories(
    foo
    PUBLIC
    "$<INSTALL_INTERFACE:${INCL_DEST}>"
)

# Package config
set(config "${PROJECT_BINARY_DIR}/FooConfig.cmake")
configure_package_config_file(
    FooConfig.cmake.in
    "${config}"
    INSTALL_DESTINATION ${CONFIG_DEST}
    PATH_VARS CONFIG_DEST
)
install(FILES "${config}" DESTINATION ${CONFIG_DEST})

# Install
install(FILES "./sources/Foo.hpp" DESTINATION ${INCL_DEST})
install(TARGETS foo DESTINATION ${LIB_DEST} EXPORT FooTargets)
install(EXPORT FooTargets NAMESPACE Foo:: DESTINATION ${CONFIG_DEST})
