# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if(DEFINED RESOURCES_IOS_IMAGES_IPAD_SUGAR_CMAKE_)
  return()
else()
  set(RESOURCES_IOS_IMAGES_IPAD_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(
    DEFAULT_IOS_IMAGES
    Launch_Landscape@2x~ipad.png
    Launch_Landscape~ipad.png
    Launch_Portrait@2x~ipad.png
    Launch_Portrait~ipad.png
)
