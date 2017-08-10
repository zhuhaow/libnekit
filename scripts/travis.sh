#!/bin/bash

set -euo pipefail

[[ "${COVERAGE:-}" ]] || COVERAGE=OFF
[[ "${COMPILE_PREFIX:-}" ]] || COMPILE_PREFIX=""

cmake -H. -Bbuild -DPLATFORM=$PLATFORM -DCOVERAGE=$COVERAGE -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/${PLATFORM}.cmake

comm="$COMPILE_PREFIX cmake --build build"
eval $comm

# Test on iOS is still unsupported.
if [[ $(perl -e "print lc('$PLATFORM');") != "ios" ]]
then
   cd build
   ctest --output-on-failure
   cd ..
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]
then
    sonar-scanner
fi
