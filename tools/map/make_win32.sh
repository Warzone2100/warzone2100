#!/bin/sh
OLDPATH="$(pwd)"
MYPATH="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
MINGW_ENV="/opt/mingw"

cd "${MYPATH}/../.."
make clean
./autogen.sh
./configure --host=i686-pc-mingw32 --enable-static PKG_CONFIG=i686-pc-mingw32-pkg-config \
        CC_FOR_BUILD="gcc" CXX_FOR_BUILD="g++" \
        CFLAGS="-pipe -m32 -march=i686 -O2 -g -gstabs -g3" \
        CXXFLAGS="-pipe -m32 -march=i686 -O2 -g -gstabs -g3 -fno-exceptions" \
        --with-installer-version="2.46.0.0" --disable-debug

cd ${MYPATH}
cmake . -B$PWD/build -DCMAKE_TOOLCHAIN_FILE=${MINGW_ENV}/usr/i686-pc-mingw32/share/cmake/mingw-cross-env-conf.cmake \
                     -DQT_IS_STATIC=YES \
                     -DWZ_SOURCE_DIR=../.. \
                     -DWZ_BUILD_DIR=../..
                     
make -C$PWD/build -j4 VERBOSE=1 all
cd ${OLDPATH}
