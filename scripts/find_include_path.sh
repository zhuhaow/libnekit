#!/usr/bin/env sh

echo | clang -x c++ -v -E - 2>&1 | sed -n '/^#include </,/^End/s|^[^/]*\([^ ]*/include[^ ]*\).*$|\1|p'
