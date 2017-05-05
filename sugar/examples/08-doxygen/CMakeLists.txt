# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

# Doxygen documentation generation example.
#
# Just run `make *-doc` after cmake done.
#
# Commands:
#     http://www.stack.nl/~dimitri/doxygen/manual/commands.html

cmake_minimum_required(VERSION 2.8.11)
project(08-example-doxygen CXX)

include("$ENV{SUGAR_ROOT}/cmake/Sugar")

include(sugar_doxygen_generate)
include(sugar_include)

include_directories("./sources")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release) # set default build type
endif()

sugar_include("./sources")

add_executable(prog ${SOURCES})

# macro expansion can be checked in attributes of class C
add_definitions(-DA_DIRECTORY_MACRO=1)

# Note that if config will be empty or differ from Debug/Release
# both A_DEBUG/A_NDEBUG macroses will not be defined
target_compile_definitions(
    prog
    PUBLIC
    A_TARGET_MACRO=1
    $<$<CONFIG:Debug>:A_DEBUG=1>
    $<$<CONFIG:Release>:A_NDEBUG=1>
)

sugar_doxygen_generate(
    DEVELOPER
    TARGET
    prog
    DOXYTARGET
    internal-doc
    DOXYFILE
    "${SUGAR_ROOT}/examples/Doxyfile.in"
)

sugar_doxygen_generate(
    TARGET
    prog
    DOXYTARGET
    public-doc
    DOXYFILE
    "${SUGAR_ROOT}/examples/Doxyfile.in"
)
