# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

add_executable(
    module_b_bin
    "./main.cpp"
    ${MODULE_A_SOURCES}
    ${MODULE_B_SOURCES}
)
