hunter_add_package(GTest)
find_package(GMock CONFIG REQUIRED)

set(SUGAR_ROOT ../sugar)
include(${SUGAR_ROOT}/cmake/Sugar)
include(sugar_add_gtest)

option(NE_ENABLE_MEMCHECK "Enable valgrind when running tests." ON)
find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS --trace-children=yes --leak-check=full)

function(add_mem_test name)
  if (CTEST_MEMORYCHECK_COMMAND AND NE_ENABLE_MEMCHECK AND NOT APPLE AND NOT ANDROID)
    message(STATUS "Adding valgrind test for ${name}.")
    set(MEMCHECK_PARAMS ${CTEST_MEMORYCHECK_COMMAND} ${CTEST_MEMORYCHECK_COMMAND_OPTIONS} $<TARGET_FILE:${name}>)
    add_test(NAME ${name} COMMAND ${MEMCHECK_PARAMS})
  elseif(NOT ANDROID AND NOT IOS)
    message(STATUS "Adding test for ${name}.")
    sugar_add_gtest(${name} ${name})
  endif()
endfunction()

include_directories(.)

add_executable(stream_coder_pipe_test stream_coder_pipe_test.cc)
target_link_libraries(stream_coder_pipe_test nekit GMock::main)
add_mem_test(stream_coder_pipe_test)

add_executable(buffer_test buffer_test.cc)
target_link_libraries(buffer_test nekit GMock::main)
add_mem_test(buffer_test)

add_executable(socks5_stream_coder_session_test socks5_stream_coder_session_test.cc)
target_link_libraries(socks5_stream_coder_session_test nekit GMock::main)
add_mem_test(socks5_stream_coder_session_test)
