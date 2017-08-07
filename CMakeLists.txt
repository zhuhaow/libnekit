cmake_minimum_required(VERSION 3.0.0)

if (CMAKE_VERSION VERSION_LESS "3.7.1")
  message(WARNING "It is highly recommended to used the latest version of CMake.")
  message(WARNING "To support Android, you should use at least 3.7.1")
endif()

project(libnekit C CXX)

set(PLATFORM "mac" CACHE STRING "Name of the platform.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

check_cxx_compiler_flag(-Wno-pragma-once-outside-header HAS_PRAGMA_ONCE_WARNING)
if(HAS_PRAGMA_ONCE_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pragma-once-outside-header")
endif()

option(COVERAGE "Generate coverage report" OFF)
if(COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

include_directories("include")
include_directories(SYSTEM "deps/${PLATFORM}/include")

set(CMAKE_FIND_ROOT_PATH "${PROJECT_SOURCE_DIR}/deps/${PLATFORM}")

if(APPLE AND NOT IOS)
  execute_process(COMMAND scripts/find_include_path.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE INCLUDE_DIRS)
  string(REPLACE "\n" ";" INCLUDE_DIRS ${INCLUDE_DIRS})
  include_directories(${INCLUDE_DIRS})
  message(STATUS "Added more include dirs for apple clang")
endif()

if(WIN32)
  add_definitions(-DELPP_WINSOCK2 -DWIN32_LEAN_AND_MEAN)
endif()

add_definitions(-DBOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

set(SOURCES
  src/stream_coder/stream_coder_pipe.cc
  src/stream_coder/socks5_server_stream_coder.cc
  src/stream_coder/direct_stream_coder.cc
  src/transport/tcp_socket.cc
  src/transport/tcp_listener.cc
  src/transport/tunnel.cc
  src/transport/tcp_connector.cc
  src/transport/connection_interface.cc
  src/utils/boost_error.cc
  src/utils/session.cc
  src/utils/system_resolver.cc
  src/utils/error.cc
  src/utils/domain.cc
  src/utils/logger.cc
  src/utils/cancelable.cc
  src/init.cc
  src/rule/rule_set.cc
  src/rule/all_rule.cc
  src/transport/direct_adapter.cc
  src/instance.cc
  src/transport/socks5_listener.cc
  )

cmake_policy(SET CMP0063 NEW)
add_library(nekit ${SOURCES})
set_target_properties(nekit PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)

find_library(BOOST boost
  NO_SYSTEM_ENVIROMENT_PATH)
if(NOT BOOST)
  message(FATAL_ERROR "Boost not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${BOOST})

find_library(OPENSSL crypto NO_SYSTEM_ENVIROMENT_PATH)
if(NOT OPENSSL)
  message(FATAL_ERROR "OpenSSL not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${OPENSSL})

target_link_libraries(nekit ${LIBS})

if (NOT IOS AND NOT ANDROID)
  enable_testing()
  add_subdirectory(test)

  add_subdirectory(app)
endif()
