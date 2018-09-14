#!/bin/bash

set -euo pipefail

[[ "${COVERAGE:-}" ]] || COVERAGE=OFF
[[ "${COMPILE_PREFIX:-}" ]] || COMPILE_PREFIX=""

compile() {
    cmake -H. -Bbuild -DPLATFORM=$PLATFORM -DCOVERAGE=$COVERAGE -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/${PLATFORM}.cmake
    return $((eval "$COMPILE_PREFIX cmake --build build"))
}

set +e
# Try to build from source if prebuild binary fails to build since the prebuild package may use newer ld which is supported on current platform
eval "pipenv run conan install . -u -if build/ $CONAN_CONFIG"
            && compile
            || eval "pipenv run conan install . -u -if build/ $CONAN_CONFIG --build=\"*\""
            && compile
            || exit 1
set -e

# Test on iOS is still unsupported.
if [[ $(perl -e "print lc('$PLATFORM');") != "ios" ]]
then
   cd build
   ctest --verbose
   cd ..
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]
then
    if [[ "$COVERAGE" == "ON" ]]
    then
        mkdir coverage
        cd coverage
        find ../build -type f -name '*.gcda' -exec gcov -pbcu {} +
        cd ..
    fi

    sonar-scanner -Dsonar.branch.name="$TRAVIS_BRANCH"
fi
