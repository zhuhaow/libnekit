#!/bin/bash

set -euo pipefail

if [ -z $PLATFORM ]; then
    echo "PLATFORM must be set."
    exit 1
fi

if [ -z $COVERAGE ]; then
    COVERAGE=OFF
fi

cmake -H. -Bbuild -DPLATFORM=$PLATFORM -DCONVERAGE=$COVERAGE
cmake --build build

# Test on iOS is still unsupported.
if [[ $(perl -e "print lc('$PLATFORM');") != "ios" ]]; then
   cd build
   ctest --output-on-failure
fi
