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

BUILD_DIR=`mktemp -d`
LOGDIR="$BUILD_DIR/logs/"
IOS_MIN_VERSION=9.0
SDKVERSION=`xcrun -sdk iphoneos --show-sdk-version`
OSX_SDKVERSION=`xcrun -sdk macosx --show-sdk-version`
DEVELOPER=`xcode-select -print-path`
XCODE_ROOT=`xcode-select -print-path`

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
: ${IPHONE_SDKVERSION:=`xcodebuild -showsdks | grep iphoneos | egrep "[[:digit:]]+\.[[:digit:]]+" -o | tail -1`}
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

cleanEverythingReadyToStart()
{
    echo Cleaning everything before we start to build...

    rm -rf iphone-build iphonesim-build osx-build
    rm -rf $IOSBUILDDIR
    rm -rf $PREFIXDIR
    rm -rf $IOSINCLUDEDIR
    rm -rf $TARBALLDIR/build
    rm -rf $LOGDIR

    doneSection
}

postcleanEverything()
{
	echo Cleaning everything after the build...

	rm -rf iphone-build iphonesim-build osx-build
  rm -rf $BUILD_DIR
	doneSection
}

prepare()
{

    mkdir -p $LOGDIR
    mkdir -p $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR_SRC
    mkdir -p $OUTPUT_DIR_LIB

}

#===============================================================================

downloadBoost()
{
    if [ ! -s $TARBALLDIR/boost_${BOOST_VERSION2}.tar.bz2 ]; then
        echo "Downloading boost ${BOOST_VERSION}"
        curl -L -o $TARBALLDIR/boost_${BOOST_VERSION2}.tar.bz2 http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION2}.tar.bz2/download
    fi

    doneSection
}

#===============================================================================

unpackBoost()
{
    [ -f "$BOOST_TARBALL" ] || abort "Source tarball missing."

    echo Unpacking boost into $SRCDIR...

    [ -d $SRCDIR ]    || mkdir -p $SRCDIR
    [ -d $BOOST_SRC ] || ( cd $SRCDIR; tar xfj $BOOST_TARBALL )
    [ -d $BOOST_SRC ] && echo "    ...unpacked as $BOOST_SRC"

    # Fix linker issue with utf8_codecvt_facet.cpp
    # copied from http://swift.im/git/swift/tree/3rdParty/Boost/update.sh#n48?id=8dce1cd03729624a25a98dd2c0d026b93e452f86
    echo Fixing utf8_codecvt_facet.cpp duplicates...

    [ -f "$BOOST_SRC/libs/filesystem/src/utf8_codecvt_facet.cpp" ] && (mv "$BOOST_SRC/libs/filesystem/src/utf8_codecvt_facet.cpp" "$BOOST_SRC/libs/filesystem/src/filesystem_utf8_codecvt_facet.cpp" )
    sed -i .bak 's/utf8_codecvt_facet/filesystem_utf8_codecvt_facet/' "$BOOST_SRC/libs/filesystem/build/Jamfile.v2"

    [ -f "$BOOST_SRC/libs/program_options/src/utf8_codecvt_facet.cpp" ] && (mv "$BOOST_SRC/libs/program_options/src/utf8_codecvt_facet.cpp" "$BOOST_SRC/libs/program_options/src/program_options_utf8_codecvt_facet.cpp" )
    sed -i .bak 's/utf8_codecvt_facet/program_options_utf8_codecvt_facet/' "$BOOST_SRC/libs/program_options/build/Jamfile.v2"

    for lib in $BOOST_LIBS; do
      rm -rf $BOOST_SRC/libs/$lib/*.doc $BOOST_SRC/libs/$lib/src/*.doc $BOOST_SRC/libs/$lib/test
    done
    rm -rf $BOOST_SRC/tools/bcp/*.html $BOOST_SRC/libs/test $BOOST_SRC/doc $BOOST_SRC/boost.png $BOOST_SRC/boost/test

    echo Fixed utf8_codecvt_facet.cpp duplicates...

    doneSection
}

#===============================================================================

restoreBoost()
{
    mv $BOOST_SRC/tools/build/example/user-config.jam.bk $BOOST_SRC/tools/build/example/user-config.jam
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

    sed -ie 's/-arch arm/-arch armv7 -arch arm64/g' "${BOOST_SRC}/tools/build/src/tools/darwin.jam"
    cp $BOOST_SRC/tools/build/example/user-config.jam $BOOST_SRC/tools/build/example/user-config.jam.bk

    cat >> $BOOST_SRC/tools/build/example/user-config.jam <<EOF
using darwin : ${IPHONE_SDKVERSION}~iphone
: $XCODE_ROOT/Toolchains/XcodeDefault.xctoolchain/usr/bin/$COMPILER -arch armv7 -arch arm64 $EXTRA_CPPFLAGS -isysroot ${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS} -I${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS}/usr/include/ -L${CROSS_TOP_IOS}/SDKs/${CROSS_SDK_IOS}/usr/lib/
: <striper> <root>$XCODE_ROOT/Platforms/iPhoneOS.platform/Developer <architecture>arm <target-os>iphone
;
using darwin : ${IPHONE_SDKVERSION}~iphonesim
: $XCODE_ROOT/Toolchains/XcodeDefault.xctoolchain/usr/bin/$COMPILER -arch i386 -arch x86_64 $EXTRA_CPPFLAGS -isysroot ${CROSS_TOP_SIM}/SDKs/${CROSS_SDK_SIM} -I${CROSS_TOP_SIM}/SDKs/${CROSS_SDK_SIM}/usr/include/
: <striper> <root>$XCODE_ROOT/Platforms/iPhoneSimulator.platform/Developer <architecture>x86 <target-os>iphone
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

    echo "------------------"
    LOG="$LOGDIR/build-iphone-install.log"
    echo "Running bjam for iphone-build install"
    echo "To see status in realtime check:"
    echo " ${LOG}"
    echo "Please stand by..."
    ./bjam -j${PARALLEL_MAKE} --build-dir=iphone-build -sBOOST_BUILD_USER_CONFIG=$BOOST_SRC/tools/build/example/user-config.jam --stagedir=iphone-build/stage --prefix=$PREFIXDIR --toolset=darwin-${IPHONE_SDKVERSION}~iphone cxxflags="-miphoneos-version-min=$IOS_MIN_VERSION -stdlib=$STDLIB $BITCODE" variant=release linkflags="-stdlib=$STDLIB" architecture=arm target-os=iphone macosx-version=iphone-${IPHONE_SDKVERSION} define=_LITTLE_ENDIAN link=static install > "${LOG}" 2>&1
    if [ $? != 0 ]; then 
        tail -n 100 "${LOG}"
        echo "Problem while Building iphone-build install - Please check ${LOG}"
        exit 1
    else 
        echo "iphone-build install successful"
    fi
    doneSection

    echo "------------------"
    LOG="$LOGDIR/build-iphone-simulator-build.log"
    echo "Running bjam for iphone-sim-build "
    echo "To see status in realtime check:"
    echo " ${LOG}"
    echo "Please stand by..."
    ./bjam -j${PARALLEL_MAKE} --build-dir=iphonesim-build -sBOOST_BUILD_USER_CONFIG=$BOOST_SRC/tools/build/example/user-config.jam --stagedir=iphonesim-build/stage --toolset=darwin-${IPHONE_SDKVERSION}~iphonesim architecture=x86 target-os=iphone variant=release cxxflags="-miphoneos-version-min=$IOS_MIN_VERSION -stdlib=$STDLIB $BITCODE" macosx-version=iphonesim-${IPHONE_SDKVERSION} link=static stage > "${LOG}" 2>&1
    if [ $? != 0 ]; then 
        tail -n 100 "${LOG}"
        echo "Problem while Building iphone-simulator build - Please check ${LOG}"
        exit 1
    else 
        echo "iphone-simulator build successful"
    fi

    doneSection
}

#===============================================================================

scrunchAllLibsTogetherInOneLibPerPlatform()
{
    cd $BOOST_SRC

    mkdir -p $IOSBUILDDIR/armv7/obj
    #mkdir -p $IOSBUILDDIR/armv7s/obj
	mkdir -p $IOSBUILDDIR/arm64/obj
    mkdir -p $IOSBUILDDIR/i386/obj
	mkdir -p $IOSBUILDDIR/x86_64/obj

    ALL_LIBS=$(find iphone-build/stage/lib -name "libboost_*.a" | sed -n 's/.*\(libboost_.*.a\)/\1/p' | paste -sd " " -)

    echo Splitting all existing fat binaries...

    for NAME in $ALL_LIBS; do

        $ARM_DEV_CMD lipo "iphone-build/stage/lib/$NAME" -thin armv7 -o $IOSBUILDDIR/armv7/$NAME
        #$ARM_DEV_CMD lipo "iphone-build/stage/lib/$NAME" -thin armv7s -o $IOSBUILDDIR/armv7s/$NAME
		$ARM_DEV_CMD lipo "iphone-build/stage/lib/$NAME" -thin arm64 -o $IOSBUILDDIR/arm64/$NAME

		$ARM_DEV_CMD lipo "iphonesim-build/stage/lib/$NAME" -thin i386 -o $IOSBUILDDIR/i386/$NAME
		$ARM_DEV_CMD lipo "iphonesim-build/stage/lib/$NAME" -thin x86_64 -o $IOSBUILDDIR/x86_64/$NAME

    done

    echo "Decomposing each architecture's .a files"

    for NAME in $ALL_LIBS; do
        echo Decomposing $NAME...
        (cd $IOSBUILDDIR/armv7/obj; ar -x ../$NAME );
        #(cd $IOSBUILDDIR/armv7s/obj; ar -x ../$NAME );
		(cd $IOSBUILDDIR/arm64/obj; ar -x ../$NAME );
        (cd $IOSBUILDDIR/i386/obj; ar -x ../$NAME );
		(cd $IOSBUILDDIR/x86_64/obj; ar -x ../$NAME );
    done

    echo "Linking each architecture into an uberlib ($ALL_LIBS => libboost.a )"

    rm $IOSBUILDDIR/*/libboost.a
    
    echo ...armv7
    (cd $IOSBUILDDIR/armv7; $ARM_DEV_CMD ar crus libboost.a obj/*.o; )
    #echo ...armv7s
    #(cd $IOSBUILDDIR/armv7s; $ARM_DEV_CMD ar crus libboost.a obj/*.o; )
    echo ...arm64
    (cd $IOSBUILDDIR/arm64; $ARM_DEV_CMD ar crus libboost.a obj/*.o; )
    echo ...i386
    (cd $IOSBUILDDIR/i386;  $SIM_DEV_CMD ar crus libboost.a obj/*.o; )
    echo ...x86_64
    (cd $IOSBUILDDIR/x86_64;  $SIM_DEV_CMD ar crus libboost.a obj/*.o; )

    echo "Making fat lib for iOS Boost $BOOST_VERSION"
    lipo -c $IOSBUILDDIR/armv7/libboost.a \
            $IOSBUILDDIR/arm64/libboost.a \
            $IOSBUILDDIR/i386/libboost.a \
            $IOSBUILDDIR/x86_64/libboost.a \
            -output $OUTPUT_DIR_LIB/libboost.a

    echo "Completed Fat Lib"
    echo "------------------"

}

#===============================================================================
buildIncludes()
{
    
    mkdir -p $IOSINCLUDEDIR
    echo "------------------"
    echo "Copying Includes to Final Dir $OUTPUT_DIR_SRC"
    LOG="$LOGDIR/buildIncludes.log"
    set +e

    cp -r $PREFIXDIR/include/boost/*  $OUTPUT_DIR_SRC/ > "${LOG}" 2>&1
    if [ $? != 0 ]; then 
        tail -n 100 "${LOG}"
        echo "Problem while copying includes - Please check ${LOG}"
        exit 1
    else 
        echo "Copy of Includes successful"
    fi
    echo "------------------"

    doneSection
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
buildIncludes

restoreBoost

postcleanEverything

echo "Completed successfully"

#===============================================================================
