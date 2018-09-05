cmake_minimum_required(VERSION 3.2.0)

if (CMAKE_VERSION VERSION_LESS "3.7.1")
  message(WARNING "It is highly recommended to used the latest version of CMake.")
  message(WARNING "To support Android, you should use at least 3.7.1")
endif()

project(libnekit C CXX)

set(PLATFORM "mac" CACHE STRING "Name of the platform.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

cmake_policy(SET CMP0063 NEW)
add_library(nekit "")

include(CheckCXXCompilerFlag)

if (NOT WIN32)
  # I can't get VS suppress warnings from thrid party libraries, tried CAExcludePath and IncludePath but got no luck.
  check_cxx_compiler_flag(-Wall HAS_WALL)
  target_compile_options(nekit PRIVATE $<$<BOOL:${HAS_WALL}>:-Wall>)

  check_cxx_compiler_flag(-Wextra HAS_WEXTRA)
  target_compile_options(nekit PRIVATE $<$<BOOL:${HAS_WEXTRA}>:-Wextra>)

  check_cxx_compiler_flag(-pedantic HAS_PEDANTIC)
  target_compile_options(nekit PRIVATE $<$<BOOL:${HAS_PEDANTIC}>:-Wpedantic>)

  check_cxx_compiler_flag(-Wno-pragma-once-outside-header HAS_PRAGMA_ONCE_WARNING)
  target_compile_options(nekit PRIVATE $<$<BOOL:${HAS_PRAGMA_ONCE_WARNING}>:-Wno-pragma-once-outside-header>)
endif()

option(COVERAGE "Generate coverage report" OFF)
if(COVERAGE)
  target_compile_options(nekit PRIVATE -g -O0 -coverage)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -coverage")
endif()

target_compile_options(nekit PRIVATE $<$<CONFIG:DEBUG>:-g -O0>)
target_compile_options(nekit PRIVATE $<$<CONFIG:RELEASE>:-O3>)

set(DEPENDENCY_ROOT "${PROJECT_SOURCE_DIR}/deps/${PLATFORM}")

target_include_directories(nekit PUBLIC "include")
target_include_directories(nekit PRIVATE "modules")
target_include_directories(nekit SYSTEM PUBLIC "${DEPENDENCY_ROOT}/include")

list(APPEND CMAKE_LIBRARY_PATH "${DEPENDENCY_ROOT}/lib")
list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "" "lib")

if(WIN32)
  target_compile_definitions(nekit PUBLIC -DELPP_WINSOCK2 -DWIN32_LEAN_AND_MEAN)
endif()

target_compile_definitions(nekit PUBLIC -DBOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

target_sources(nekit PRIVATE
  src/transport/tcp_socket.cc
  src/transport/tcp_listener.cc
  src/transport/tunnel.cc
  src/transport/tcp_connector.cc
  src/utils/system_resolver.cc
  src/utils/timer.cc
  src/utils/logger.cc
  src/utils/cancelable.cc
  src/utils/maxmind.cc
  src/utils/subnet.cc
  src/utils/country_iso_code.cc
  src/utils/http_message_stream_rewriter.cc
  src/init.cc
  src/proxy_manager.cc
  src/rule/rule_manager.cc
  src/rule/all_rule.cc
  src/rule/dns_fail_rule.cc
  src/rule/geo_rule.cc
  src/rule/domain_rule.cc
  src/rule/domain_regex_rule.cc
  src/rule/subnet_rule.cc
  src/instance.cc
  src/crypto/random.cc
  src/crypto/key_generator.cc
  src/crypto/stream_cipher_interface.cc
  src/crypto/tls_tunnel.cc
  src/utils/buffer.cc
  src/utils/endpoint.cc
  src/utils/stream_reader.cc
  src/data_flow/socks5_server_data_flow.cc
  src/data_flow/http_server_data_flow.cc
  src/data_flow/http_data_flow.cc
  src/data_flow/socks5_data_flow.cc
  src/data_flow/speed_data_flow.cc
  src/data_flow/tls_data_flow.cc
  modules/http_parser/http_parser.c
  )

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/contrib" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/contrib" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/CMakeLists.txt")
  message(STATUS "Add contrib folder.")
  add_subdirectory("contrib")
endif()

find_library(BOOST boost NO_SYSTEM_ENVIRONMENT_PATH CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT BOOST)
  message(FATAL_ERROR "Boost not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${BOOST})

find_library(OPENSSL_CRYPTO crypto NO_SYSTEM_ENVIRONMENT_PATH CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT OPENSSL_CRYPTO)
  message(FATAL_ERROR "OpenSSL not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${OPENSSL_CRYPTO})

find_library(OPENSSL_SSL ssl NO_SYSTEM_ENVIRONMENT_PATH CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT OPENSSL_SSL)
  message(FATAL_ERROR "OpenSSL not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${OPENSSL_SSL})

find_library(SODIUM sodium NO_SYSTEM_ENVIRONMENT_PATH CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT SODIUM)
  message(FATAL_ERROR "libsodium not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${SODIUM})

find_library(MAXMINDDB maxminddb NO_SYSTEM_ENVIRONMENT_PATH CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT MAXMINDDB)
  message(FATAL_ERROR "maxminddb not found. Build dependency first.")
endif()
set(LIBS ${LIBS} ${MAXMINDDB})

target_link_libraries(nekit ${LIBS})

if (NOT IOS AND NOT ANDROID)
  enable_testing()
  add_subdirectory(test)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/app" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/app")
endif()
