# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if(DEFINED RESOURCES_IOS_IMAGES_IPHONE_SUGAR_CMAKE_)
  return()
else()
  set(RESOURCES_IOS_IMAGES_IPHONE_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(
    DEFAULT_IOS_IMAGES
    Default-568h@2x.png
    Launch_Portrait@2x.png
)
