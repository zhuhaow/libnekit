# Copyright (c) 2016, Ruslan Baratov
# All rights reserved.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

include(fatal_error)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "OS X Deployment target" FORCE)
