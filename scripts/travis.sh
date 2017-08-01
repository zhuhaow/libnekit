#!/bin/bash

set -euo pipefail

[[ "${COVERAGE:-}" ]] || COVERAGE=OFF

cmake -H. -Bbuild -DPLATFORM=$PLATFORM -DCOVERAGE=$COVERAGE -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/${PLATFORM}.cmake
cmake --build build

# Test on iOS is still unsupported.
if [[ $(perl -e "print lc('$PLATFORM');") != "ios" ]]; then
   cd build
   ctest --output-on-failure
fi
