#!/bin/bash
#
#===============================================================================
# Filename:  build-libc++.sh
# Author:    Pete Goodliffe, Daniel Rosser
# Copyright: (c) Copyright 2009 Pete Goodliffe, 2013-2015 Daniel Rosser
# Licence:   Please feel free to use this, with attribution
# Modified version ## for ofxiOSBoost
#===============================================================================
#
# Builds a Boost framework for the iPhone.
# Creates a set of universal libraries that can be used on an iPhone and in the
# iPhone simulator. Then creates a pseudo-framework to make using boost in Xcode
# less painful.
#
# To configure the script, define:
#    BOOST_LIBS:        which libraries to build
#    IPHONE_SDKVERSION: iPhone SDK version (e.g. 8.0)
#
# Then go get the source tar.bz of the boost you want to build, shove it in the
# same directory as this script, and run "./boost.sh". Grab a cuppa. And voila.
#===============================================================================

CPPSTD=c++11    #c++89, c++99, c++14
STDLIB=libc++   # libstdc++
COMPILER=clang++
PARALLEL_MAKE=4   # how many threads to make boost with

BITCODE="-fembed-bitcode"  # Uncomment this line for Bitcode generation

BUILD_DIR=$(mktemp -d)
LOGDIR="$BUILD_DIR/logs/"
IOS_MIN_VERSION=9.0
SDKVERSION=$(xcrun -sdk iphoneos --show-sdk-version)
OSX_SDKVERSION=$(xcrun -sdk macosx --show-sdk-version)
DEVELOPER=$(xcode-select -print-path)
XCODE_ROOT=$(xcode-select -print-path)

if [ ! -d "$DEVELOPER" ]; then
  echo "xcode path is not set correctly $DEVELOPER does not exist (most likely because of xcode > 4.3)"
  echo "run"
  echo "sudo xcode-select -switch <xcode path>"
  echo "for default installation:"
  echo "sudo xcode-select -switch /Applications/Xcode.app/Contents/Developer"
  exit 1
fi

case $DEVELOPER in  
     *\ * )
           echo "Your Xcode path contains whitespaces, which is not supported."
           exit 1
          ;;
esac

: ${BOOST_LIBS:="random regex graph random chrono thread signals filesystem system date_time locale"}
: ${IPHONE_SDKVERSION:=$(xcodebuild -showsdks | grep iphoneos | egrep "[[:digit:]]+\.[[:digit:]]+" -o | tail -1)}
: ${EXTRA_CPPFLAGS:="-fPIC -DBOOST_SP_USE_SPINLOCK -std=$CPPSTD -stdlib=$STDLIB -miphoneos-version-min=$IOS_MIN_VERSION $BITCODE -fvisibility=hidden -fvisibility-inlines-hidden"}

: ${IOSBUILDDIR:=$BUILD_DIR/libs/boost/lib}
: ${IOSINCLUDEDIR:=$BUILD_DIR/libs/boost/include/boost}
: ${PREFIXDIR:=$BUILD_DIR/ios/prefix}
: ${OUTPUT_DIR:=$BUILD_DIR/output/}
: ${OUTPUT_DIR_LIB:=$OUTPUT_DIR/lib/}
: ${OUTPUT_DIR_SRC:=$OUTPUT_DIR/include/boost}

BOOST_INCLUDE=$BOOST_SRC/boost


#===============================================================================
ARM_DEV_CMD="xcrun --sdk iphoneos"
SIM_DEV_CMD="xcrun --sdk iphonesimulator"
OSX_DEV_CMD="xcrun --sdk macosx"

#===============================================================================


#===============================================================================
# Functions
#===============================================================================

abort()
{
    echo
    echo "Aborted: $@"
    exit 1
}

doneSection()
{
    echo
    echo "================================================================="
    echo "Done"
    echo
}

#===============================================================================

prepare()
{

    mkdir -p $LOGDIR
    mkdir -p $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR_SRC
    mkdir -p $OUTPUT_DIR_LIB

}

#===============================================================================

updateBoost()
{
    echo Updating boost into $BOOST_SRC...
    local CROSS_TOP_IOS="${DEVELOPER}/Platforms/iPhoneOS.platform/Developer"
    local CROSS_SDK_IOS="iPhoneOS${SDKVERSION}.sdk"
    local CROSS_TOP_SIM="${DEVELOPER}/Platforms/iPhoneSimulator.platform/Developer"
    local CROSS_SDK_SIM="iPhoneSimulator${SDKVERSION}.sdk"
    local BUILD_TOOLS="${DEVELOPER}"

    sed -ie 's/-arch arm/-arch arm64/g' "${BOOST_SRC}/tools/build/src/tools/darwin.jam"
    cp $BOOST_SRC/tools/build/example/user-config.jam $BOOST_SRC/tools/build/example/user-config.jam.bk

    cat >> $BOOST_SRC/tools/build/example/user-config.jam <<EOF
using darwin : ${IPHONE_SDKVERSION}~iphone
: $XCODE_ROOT/Toolchains/XcodeDefault.xctoolchain/usr/bin/$COMPILER -arch arm64 $EXTRA_CPPFLAGS -isysroot ${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS} -I${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS}/usr/include/ -L${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS}/usr/lib/
: <striper> <root>$XCODE_ROOT/Platforms/iPhoneOS.platform/Developer <architecture>arm <target-os>iphone
;
EOF

    doneSection
}

#===============================================================================

inventMissingHeaders()
{
    # These files are missing in the ARM iPhoneOS SDK, but they are in the simulator.
    # They are supported on the device, so we copy them from x86 SDK to a staging area
    # to use them on ARM, too.
    echo Invent missing headers

    cp $XCODE_ROOT/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator${IPHONE_SDKVERSION}.sdk/usr/include/{crt_externs,bzlib}.h $BOOST_SRC
}

#===============================================================================

bootstrapBoost()
{
    cd $BOOST_SRC

    BOOST_LIBS_COMMA=$(echo $BOOST_LIBS | sed -e "s/ /,/g")
    echo "Bootstrapping (with libs $BOOST_LIBS_COMMA)"
    ./bootstrap.sh --with-libraries=$BOOST_LIBS_COMMA

    doneSection
}

#===============================================================================

buildBoostForIPhoneOS()
{
    cd $BOOST_SRC

    # Install this one so we can copy the includes for the frameworks...

    
    set +e    
    echo "------------------"
    LOG="$LOGDIR/build-iphone-stage.log"
    echo "Running bjam for iphone-build stage"
    echo "To see status in realtime check:"
    echo " ${LOG}"
    echo "Please stand by..."
    ./bjam -j${PARALLEL_MAKE} --build-dir=iphone-build -sBOOST_BUILD_USER_CONFIG=$BOOST_SRC/tools/build/example/user-config.jam --stagedir=iphone-build/stage --prefix=$PREFIXDIR --toolset=darwin-${IPHONE_SDKVERSION}~iphone cxxflags="-miphoneos-version-min=$IOS_MIN_VERSION -stdlib=$STDLIB $BITCODE" variant=release linkflags="-stdlib=$STDLIB" architecture=arm target-os=iphone macosx-version=iphone-${IPHONE_SDKVERSION} define=_LITTLE_ENDIAN link=static stage > "${LOG}" 2>&1
    if [ $? != 0 ]; then 
        tail -n 100 "${LOG}"
        echo "Problem while Building iphone-build stage - Please check ${LOG}"
        exit 1
    else 
        echo "iphone-build stage successful"
    fi

    doneSection
}

#===============================================================================

scrunchAllLibsTogetherInOneLibPerPlatform()
{
    cd $BOOST_SRC

	  mkdir -p $IOSBUILDDIR/arm64/obj

    ALL_LIBS=$(find iphone-build/stage/lib -name "libboost_*.a" | sed -n 's/.*\(libboost_.*.a\)/\1/p' | paste -sd " " -)

    echo "Decomposing each architecture's .a files"

    for NAME in $ALL_LIBS; do
        echo Decomposing $NAME...
		    (cd $IOSBUILDDIR/arm64/obj; ar -x "$BOOST_SRC/iphone-build/stage/lib/$NAME" );
    done

    echo "Linking each architecture into an uberlib ($ALL_LIBS => libboost.a )"

    echo ...arm64
    (cd $IOSBUILDDIR/arm64; $ARM_DEV_CMD ar crus libboost.a obj/*.o; )

    echo "Making fat lib for iOS Boost $BOOST_VERSION"
    lipo -c $IOSBUILDDIR/arm64/libboost.a \
            -output $OUTPUT_DIR_LIB/libboost.a

    echo "Completed Fat Lib"
    echo "------------------"

}

#===============================================================================
# Execution starts here
#===============================================================================

mkdir -p $IOSBUILDDIR

# cleanEverythingReadyToStart #may want to comment if repeatedly running during dev
# restoreBoost

echo "BOOST_LIBS:        $BOOST_LIBS"
echo "BOOST_SRC:         $BOOST_SRC"
echo "IOSBUILDDIR:       $IOSBUILDDIR"
echo "PREFIXDIR:         $PREFIXDIR"
echo "IPHONE_SDKVERSION: $IPHONE_SDKVERSION"
echo "XCODE_ROOT:        $XCODE_ROOT"
echo "COMPILER:          $COMPILER"
if [ -z ${BITCODE} ]; then
    echo "BITCODE EMBEDDED: NO $BITCODE"
else
    echo "BITCODE EMBEDDED: YES with: $BITCODE"
fi

if [ -z ${BOOST_SRC} ]; then
    echo "ERROR: BOOST_SRC must be set to the root of boost source directory."
    exit 1
fi

inventMissingHeaders
prepare
bootstrapBoost
updateBoost
buildBoostForIPhoneOS
scrunchAllLibsTogetherInOneLibPerPlatform

echo "Completed successfully"

#===============================================================================
