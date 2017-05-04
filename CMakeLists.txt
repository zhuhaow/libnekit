cmake_minimum_required(VERSION 3.0.0)

if (CMAKE_VERSION VERSION_LESS "3.7.1")
  message(WARNING "It is highly recommended to used the latest version of CMake.")
  message(WARNING "To support Android, you should use at least 3.7.1")
endif()

option(HUNTER_STATUS_PRINT "Print build information in hunter" OFF)
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.18.44.tar.gz"
    SHA1 "a78f0b377b8e53c038f12fc18b0c02564c4534c8"
    LOCAL
)

project(libnekit CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wall HAS_WALL)
if(HAS_WALL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

check_cxx_compiler_flag(-Wextra HAS_WEXTRA)
if(HAS_WEXTRA)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
endif()

check_cxx_compiler_flag(-pedantic HAS_PEDANTIC)
if(HAS_PEDANTIC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif()

option(COVERAGE "Generate coverage report" OFF)
if(COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

include_directories("include")

if(APPLE)
  execute_process(COMMAND scripts/find_include_path.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE INCLUDE_DIRS)
  string(REPLACE "\n" ";" INCLUDE_DIRS ${INCLUDE_DIRS})
  include_directories(${INCLUDE_DIRS})
  message(STATUS "Added more include dirs for apple clang")
endif()

if(WIN32)
  add_definitions(-DELPP_WINSOCK2 -DWIN32_LEAN_AND_MEAN)
endif()

hunter_add_package(Boost)
find_package(Boost CONFIG REQUIRED)

set(SOURCES
  src/stream_coder/stream_coder_pipe.cc
  src/deps/easylogging++.cc
  )

add_library(nekit ${SOURCES})
target_compile_features(nekit PUBLIC cxx_lambdas)
target_compile_features(nekit PUBLIC cxx_decltype)
target_compile_features(nekit PUBLIC cxx_auto_type)

set(LIBS ${LIBS} Boost::boost)
target_link_libraries(nekit ${LIBS})

enable_testing()
add_subdirectory(test)
