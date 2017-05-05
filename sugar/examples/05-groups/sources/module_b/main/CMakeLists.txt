# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_groups_generate)

set(
    SOURCES_B
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    ${MODULE_A_SOURCES}
    ${MODULE_B_SOURCES}
    ${SUGAR_SOURCES}
)
sugar_groups_generate(${SOURCES_B})

add_executable(module_b_bin ${SOURCES_B})
