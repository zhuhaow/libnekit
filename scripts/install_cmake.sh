#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

curl https://cmake.org/files/v3.9/cmake-3.9.0-Linux-x86_64.sh -o $DIR/../cmake.sh

mkdir -p $DIR/../deps/cmake

sh $DIR/../cmake.sh --prefix=$DIR/../deps/cmake --skip-license

