# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if(DEFINED RESOURCES_IOS_ICONS_SUGAR_CMAKE_)
  return()
else()
  set(RESOURCES_IOS_ICONS_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_include(app)
sugar_include(settings)
sugar_include(spotlight)
