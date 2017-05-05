cmake_minimum_required (VERSION 2.8)
project(Sugar)

include(CMakePackageConfigHelpers)
configure_package_config_file(sugar-config.cmake.in ${CMAKE_BINARY_DIR}/sugar-config.cmake INSTALL_DESTINATION share/sugar/)

install(DIRECTORY cmake DESTINATION share/sugar/)
install(FILES ${CMAKE_BINARY_DIR}/sugar-config.cmake DESTINATION share/sugar/)
