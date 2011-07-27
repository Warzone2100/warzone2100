#!/bin/sh

OLDPATH="$(pwd)"
MYPATH="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"

cd ${MYPATH}
cmake . -B$PWD/build -DCMAKE_BUILD_TYPE=Debug3
make -C$PWD/build -j4 VERBOSE=1 all
cd ${OLDPATH}
