# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(sugar_groups_generate)

set(
    SOURCES_A
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    ${MODULE_A_SOURCES}
    ${MODULE_B_SOURCES}
    ${SUGAR_SOURCES}
)
sugar_groups_generate(${SOURCES_A})

add_executable(module_a_bin ${SOURCES_A})
