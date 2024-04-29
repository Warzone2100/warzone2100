#!/bin/bash

TARGET_BUILD_ARCH="$("${CRAFT_PART_SRC}/.ci/snap/snapcraft_build_get_target_arch.sh")"

echo "[install_vulkan_sdk.sh]"
echo "Target Arch: $TARGET_BUILD_ARCH"

if [[ "$TARGET_BUILD_ARCH" == "amd64" && "$CRAFT_ARCH_BUILD_ON" == "$TARGET_BUILD_ARCH" ]]; then
  # Install Vulkan SDK (binary package)
  echo "Installing Vulkan SDK"

  # Add Vulkan SDK repo
  wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc
  wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.268-jammy.list https://packages.lunarg.com/vulkan/1.3.268/lunarg-vulkan-1.3.268-jammy.list
  apt update
  apt install --yes vulkan-sdk

else
  SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}" 2>/dev/null||echo $0)"
  SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
  source "${SCRIPT_DIR}/../vulkan/install_glslc.sh"
fi

echo "Installing Vulkan SDK - done"
