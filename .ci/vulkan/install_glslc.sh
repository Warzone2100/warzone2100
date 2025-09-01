#!/bin/bash

echo "[install_glslc.sh]"

# Compile the necessary components of the Vulkan SDK from source
echo "Compiling GLSLC"

mkdir tmp_vulkan_sdk_build
cd "tmp_vulkan_sdk_build"

# glslc
DEBIAN_FRONTEND=noninteractive apt-get -y install cmake python3 lcov

# glslc - clone
echo "Fetching shaderc"
git clone https://github.com/google/shaderc shaderc
cd shaderc
git checkout tags/v2025.3 -b v2025.3
./utils/git-sync-deps
cd ..

# glslc - cmake build + install
mkdir shaderc_build
cd shaderc_build
echo "Configuring shaderc"
cmake -GNinja -DSHADERC_SKIP_TESTS=ON -DCMAKE_BUILD_TYPE=Release ../shaderc/
echo "Compiling shaderc"
cmake --build . --target install
cd ..

cd ..
rm -rf "tmp_vulkan_sdk_build"

echo "Installing GLSLC - done"
