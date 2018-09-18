#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

curl https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh -o $DIR/../../cmake.sh

mkdir -p $DIR/../../cmake

sh $DIR/../../cmake.sh --prefix=$DIR/../../cmake --skip-license
