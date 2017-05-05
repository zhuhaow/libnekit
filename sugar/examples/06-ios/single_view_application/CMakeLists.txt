# Copyright (c) 2013-2014, Ruslan Baratov
# All rights reserved.

# iOS project example.
# Create project as Xcode: iOS -> Application -> Single View Application

cmake_minimum_required(VERSION 2.8.11)
project(06-example-ios.single_view_application CXX)

### Emulate toolchain
set(CMAKE_OSX_SYSROOT "iphoneos")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
### -- end

include("$ENV{SUGAR_ROOT}/cmake/Sugar")

include(sugar_include)

include_directories("${SUGAR_ROOT}/examples/third_party")

sugar_include("./sources")
sugar_include("${SUGAR_ROOT}/examples/resources/ios/icons")
sugar_include("${SUGAR_ROOT}/examples/resources/ios/images")
sugar_include("${SUGAR_ROOT}/examples/resources/ios/storyboards")

set(RESOURCES ${IOS_ICONS} ${DEFAULT_IOS_IMAGES} ${IOS_STORYBOARDS_RESOURCES})

if(NOT XCODE_VERSION)
  sugar_fatal_error("Xcode only")
endif()

add_executable(example ${SOURCES} ${RESOURCES})

set_target_properties(
    example
    PROPERTIES
    MACOSX_BUNDLE YES
    MACOSX_BUNDLE_INFO_PLIST
        "${SUGAR_ROOT}/examples/plist/ios/storyboard.plist.in"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # Universal (iPad + iPhone)
        # http://stackoverflow.com/a/20982506/2288008
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_INSTALL_PATH "${CMAKE_BINARY_DIR}/ProductRelease"
        # By default this setting is empty =>
        # archives not showing up in Xcode organizer.
        # http://stackoverflow.com/a/8102602/2288008
    XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "NO" # If this setting is "YES"
        # application can't pass archive validation.
        # http://stackoverflow.com/a/24040412/2288008
    RESOURCE "${RESOURCES}"
)

set_target_properties(
    example
    PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_NAME
    "Example"
    XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER
    "com.example.sugar.ios.SingleViewApplication"
)

set_target_properties(
    example
    PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_NAME[variant=Debug]
    "Example-Dbg"
    XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER[variant=Debug]
    "com.example.sugar.ios.SingleViewApplication.debug"
)

target_link_libraries(
    example
    "-framework CoreGraphics"
    "-framework Foundation"
    "-framework UIKit"
)
