#!/bin/bash

TARGET_BUILD_ARCH="$("${CRAFT_PART_SRC}/.ci/snap/snapcraft_build_get_target_arch.sh")"

echo "[install_vulkan_sdk.sh]"
echo "Target Arch: $TARGET_BUILD_ARCH"

if [[ "$TARGET_BUILD_ARCH" == "amd64" && "$CRAFT_ARCH_BUILD_ON" == "$TARGET_BUILD_ARCH" ]]; then
  # Install Vulkan SDK (binary package)
  echo "Installing Vulkan SDK"
  
  VULKANSDK_SHA256="4b41e3b30e8aedaa5dac7c136561ab463eb316a25a54e2c6245f2c299ea1fb85"
  VULKANSDK_DLURL="https://sdk.lunarg.com/sdk/download/1.4.357.1/linux/vulkansdk-linux-x86_64-1.4.357.1.tar.xz?Human=true"
  
  VULKANSDK_INSTALL_PATH="${CRAFT_PART_BUILD}/dep_tmp/vulkan_sdk"
  VULKAN_DL_FILE="${CRAFT_PART_BUILD}/dl_tmp/vulkansdk-linux-x86_64.tar.xz"
  mkdir -p "${CRAFT_PART_BUILD}/dl_tmp"
  
  wget -O "${VULKAN_DL_FILE}" "${VULKANSDK_DLURL}"
  
  echo "${VULKANSDK_SHA256} ${VULKAN_DL_FILE}" | sha256sum -c
  mkdir -p "${VULKANSDK_INSTALL_PATH}"
  tar -C "${VULKANSDK_INSTALL_PATH}" -xf "${VULKAN_DL_FILE}"
  rm "${VULKAN_DL_FILE}"
  
  export VULKAN_SDK="${VULKANSDK_INSTALL_PATH}/1.4.328.1/x86_64"
  export PATH="$PATH:${VULKAN_SDK}/bin"

else
  SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}" 2>/dev/null||echo $0)"
  SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
  source "${SCRIPT_DIR}/../vulkan/install_glslc.sh"
fi

echo "Installing Vulkan SDK - done"
