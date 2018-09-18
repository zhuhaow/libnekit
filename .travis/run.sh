#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    if which pyenv > /dev/null; then
        eval "$(pyenv init -)"
    fi
    pyenv activate conan
fi

if [[ ! -f ~/.conan/profiles/default ]]; then
   conan profile new default --detect
fi

cp conan/ios ~/.conan/profiles

python build.py
