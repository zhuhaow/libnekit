#!/bin/bash

set -euo pipefail

eval "pipenv run conan install . -u -if build/ $CONAN_CONFIG --build"

cmake -H. -Bbuild -DCOVERAGE=ON
build-wrapper-linux-x86-64 --out-dir bw-output cmake --build build

cd build
ctest --verbose
cd ..

mkdir coverage
cd coverage
find ../build -type f -name '*.gcda' -exec gcov -pbcu {} +
cd ..

sonar-scanner -Dsonar.branch.name="$TRAVIS_BRANCH"
