#!/bin/bash

TARGET_BUILD_ARCH="$("${SNAPCRAFT_PART_SRC}/.ci/snap/snapcraft_build_get_target_arch.sh")"

echo "[install_vulkan_sdk.sh]"
echo "Target Arch: $TARGET_BUILD_ARCH"

if [ "$TARGET_BUILD_ARCH" = "amd64" ]; then
  # Install Vulkan SDK (binary package)
  echo "Installing Vulkan SDK"

  # Add Vulkan SDK repo
  wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
  wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.162-bionic.list https://packages.lunarg.com/vulkan/1.2.162/lunarg-vulkan-1.2.162-bionic.list
  apt update
  apt install --yes vulkan-sdk

elif [ "$TARGET_BUILD_ARCH" = "i386" ] || [ "$TARGET_BUILD_ARCH" = "arm64" ] || [ "$TARGET_BUILD_ARCH" = "ppc64el" ] || [ "$TARGET_BUILD_ARCH" = "s390x" ]; then
  # Compile the necessary components of the Vulkan SDK from source
  echo "Compiling Vulkan SDK (selected components)"

  mkdir tmp_vulkan_sdk_build
  cd "tmp_vulkan_sdk_build"

  # glslc
  DEBIAN_FRONTEND=noninteractive apt-get -y install cmake python3 lcov

  # glslc - clone
  echo "Fetching shaderc"
  git clone https://github.com/google/shaderc shaderc
  cd shaderc
  git checkout tags/v2020.4 -b v2020.4
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

else
  # Skip Vulkan SDK
  echo "Skipping Vulkan SDK (for TARGET_BUILD_ARCH: $TARGET_BUILD_ARCH)"
  exit 0

fi

echo "Installing Vulkan SDK - done"
