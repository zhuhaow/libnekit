# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

if(DEFINED RESOURCES_SUGAR_CMAKE)
  return()
else()
  set(RESOURCES_SUGAR_CMAKE 1)
endif()

include(sugar_files)

sugar_files(
    IOS_STORYBOARDS_RESOURCES
    MainStoryboard_iPad.storyboard
    MainStoryboard_iPhone.storyboard
)
