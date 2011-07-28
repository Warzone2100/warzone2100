#!/bin/sh

OLDPATH="$(pwd)"
MYPATH="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"

cd ${MYPATH}
cmake . -B$PWD/build -DWZ_SOURCE_DIR=../.. \
                     -DWZ_BUILD_DIR=../.
make -C$PWD/build -j4 VERBOSE=1 all
cd ${OLDPATH}
