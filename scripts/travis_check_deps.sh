#!/usr/bin/env bash

if [ -z $PLATFORM ]; then
    echo "PLATFORM must be set."
    exit 1
fi

diff -rqyl scripts deps/scripts

if [ $? -ne 0 ]; then
    set -euo pipefail

    pipenv run ./scripts/build_deps.py $PLATFORM
    rm -rf deps/scripts
    cp -R scripts deps/
fi
