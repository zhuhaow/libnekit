list(APPEND CMAKE_LIBRARY_PATH "../deps/${PLATFORM}/lib")

find_package(Threads REQUIRED)

find_library(GTEST gtest NO_SYSTEM_ENVIROMENT_PATH)
find_library(GMOCK gmock NO_SYSTEM_ENVIROMENT_PATH)
find_library(GMOCK_MAIN gmock_main NO_SYSTEM_ENVIROMENT_PATH)
if(NOT GMOCK_MAIN OR NOT GMOCK OR NOT GTEST)
  message(FATAL_ERROR "GoogleTest not found. Build dependency first.")
endif()

set(LIBS ${GTEST} ${GMOCK} ${GMOCK_MAIN} ${CMAKE_THREAD_LIBS_INIT})

option(NE_ENABLE_MEMCHECK "Enable valgrind when running tests." ON)
find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS --trace-children=yes --leak-check=full)

function(add_mem_test name)
  if (CTEST_MEMORYCHECK_COMMAND AND NE_ENABLE_MEMCHECK)
    message(STATUS "Adding valgrind test for ${name}.")
    set(MEMCHECK_PARAMS ${CTEST_MEMORYCHECK_COMMAND} ${CTEST_MEMORYCHECK_COMMAND_OPTIONS} $<TARGET_FILE:${name}>)
    add_test(NAME ${name} COMMAND ${MEMCHECK_PARAMS})
  else()
    message(STATUS "Adding test for ${name}.")
    add_test(${name} ${name})
  endif()
endfunction()

include_directories(.)

add_executable(stream_coder_pipe_test stream_coder_pipe_test.cc)
target_link_libraries(stream_coder_pipe_test nekit ${LIBS})
add_mem_test(stream_coder_pipe_test)

add_executable(buffer_test buffer_test.cc)
target_link_libraries(buffer_test nekit ${LIBS})
add_mem_test(buffer_test)

add_executable(socks5_server_stream_coder_test socks5_server_stream_coder_test.cc)
target_link_libraries(socks5_server_stream_coder_test nekit ${LIBS})
add_mem_test(socks5_server_stream_coder_test)

add_executable(trie_test trie_test.cc)
target_link_libraries(trie_test nekit ${LIBS})
add_mem_test(trie_test)

add_executable(maxmind_test maxmind_test.cc)
target_link_libraries(maxmind_test nekit ${LIBS})
add_mem_test(maxmind_test)

configure_file(GeoLite2-Country.mmdb GeoLite2-Country.mmdb COPYONLY)
