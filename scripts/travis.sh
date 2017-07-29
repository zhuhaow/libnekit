#!/bin/bash

set -euo pipefail

[[ "${COVERAGE:-}" ]] || COVERAGE=OFF

cmake -H. -Bbuild -DPLATFORM=$PLATFORM -DCOVERAGE=$COVERAGE
cmake --build build

# Test on iOS is still unsupported.
if [[ $(perl -e "print lc('$PLATFORM');") != "ios" ]]; then
   cd build
   ctest --output-on-failure
fi
