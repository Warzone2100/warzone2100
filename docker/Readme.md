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
docker run --rm -v ${pwd}:/code <build_image_name> \
        ./configure --host=i686-w64-mingw32.static --enable-static \
        CC_FOR_BUILD="gcc" CXX_FOR_BUILD="g++" \
        CFLAGS="-pipe -m32 -march=i686 -O2 -g -gstabs -g3" \
        CXXFLAGS="-pipe -m32 -march=i686 -O2 -g -gstabs -g3 -fno-exceptions" \
        --enable-installer --with-installer-extdir="/mingw-cross-env/usr/i686-w64-mingw32.static" \
        --with-installer-version="2.46.0.0" --disable-debug
