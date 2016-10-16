How to build docker images (all platforms):

in the Dockerfile directory
docker build -t <build_image_name> .

How to build:
Beware of line ending mismatch between windows and linux when cloning repo.
Replace ${pwd} with your pwd command on your platform.

* ubuntu
docker run --rm -v ${pwd}:/code <build_image_name> ./autogen.sh

docker run --rm -v ${pwd}:/code <build_image_name> ./configure

docker run --rm -v ${pwd}:/code <build_image_name> make


* cross compile
docker run --rm -v ${pwd}:/code <build_image_name> -it bash

mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=/mxe/usr/i686-w64-mingw32.static/share/cmake/mxe-conf.cmake -DINSTALLER_VERSION=2.46.0.0 -DINSTALLER_EXTDIR="/mingw-cross-env/usr/i686-w64-mingw32.static" ..

make -j8
