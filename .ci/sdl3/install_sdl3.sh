#!/bin/bash

echo "[install_sdl3.sh]"

echo "Downloading SDL3 source"

# Download, build, & install SDL3 from source
SDL3_VERSION="3.2.28"
SDL3_SHA256="1330671214d146f8aeb1ed399fc3e081873cdb38b5189d1f8bb6ab15bbc04211"
SDL3_DLURL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"

mkdir tmp_sdl3_build
cd "tmp_sdl3_build"

SDL3_DL_FILE="dl_tmp/SDL3-${SDL3_VERSION}.tar.gz"
mkdir "dl_tmp"

wget -O "${SDL3_DL_FILE}" "${SDL3_DLURL}"

echo "${SDL3_SHA256} ${SDL3_DL_FILE}" | sha256sum -c

echo "Extracting SDL3 source"

mkdir "extract_tmp"
tar -C extract_tmp -xzf "${SDL3_DL_FILE}"
rm "${SDL3_DL_FILE}"

echo "Compiling SDL3"

cmake -S "./extract_tmp/SDL3-${SDL3_VERSION}" -B build -DSDL_DEPS_SHARED:BOOL=ON -DSDL_SHARED:BOOL=OFF -DSDL_STATIC:BOOL=ON -DSDL_TEST_LIBRARY:BOOL=OFF -DSDL_ALSA:BOOL=OFF -DSDL_OSS:BOOL=OFF \
&& cmake --build build \
&& cmake --install build --prefix /usr/local

cd ..
rm -rf "tmp_sdl3_build"

echo "Installing SDL3 - done"
