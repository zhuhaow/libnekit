# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

if(DEFINED RESOURCES_IOS_IMAGES_SUGAR_CMAKE)
  return()
else()
  set(RESOURCES_IOS_IMAGES_SUGAR_CMAKE 1)
endif()

include(sugar_include)

sugar_include(ipad)
sugar_include(iphone)
