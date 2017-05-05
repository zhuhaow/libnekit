#!/bin/bash -e

set -x

rm -rf ./leathers

./sugar-generate-warnings.py

trash.py ${GITENV_ROOT}/ruslo/leathers/Source/leathers
mkdir ${GITENV_ROOT}/ruslo/leathers/Source/leathers

mv leathers/* ${GITENV_ROOT}/ruslo/leathers/Source/leathers

rmdir leathers

rm wiki-table.txt

mv sugar_generate_warning_flag_by_name.cmake ../cmake/utility/
mv sugar_get_all_xcode_warning_attrs.cmake ../cmake/utility/
mv sugar_generate_warning_xcode_attr_by_name.cmake ../cmake/utility/
mv sugar_warning_unpack_one.cmake ../cmake/utility/

git status
