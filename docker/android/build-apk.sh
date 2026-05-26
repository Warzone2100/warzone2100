#!/usr/bin/env bash
# Build a Warzone 2100 Android APK using Docker.
# Usage: ./docker/android/build-apk.sh [Debug|Release]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_TYPE="${1:-Debug}"
OUTPUT_DIR="${REPO_ROOT}/output/android"
IMAGE_TAG="wz2100-android"
VCPKG_VOLUME="wz2100-vcpkg-android-cache"
GRADLE_VOLUME="wz2100-gradle-android-cache"
CCACHE_VOLUME="wz2100-ccache-android-cache"

mkdir -p "${OUTPUT_DIR}"

echo "==> Building Docker image: ${IMAGE_TAG}"
docker build \
  --platform linux/amd64 \
  --build-arg UID="$(id -u)" \
  -f "${SCRIPT_DIR}/Dockerfile" \
  -t "${IMAGE_TAG}" \
  "${REPO_ROOT}"

# Create a named volume if it doesn't already exist, fixing ownership on first creation.
create_volume_if_missing() {
  local vol="$1" mountpoint="$2"
  if ! docker volume inspect "${vol}" > /dev/null 2>&1; then
    echo "==> Creating cache volume: ${vol}"
    docker volume create "${vol}" > /dev/null
    docker run --rm --user root \
      --platform linux/amd64 \
      -v "${vol}:${mountpoint}" \
      "${IMAGE_TAG}" \
      chown -R warzone2100:root "${mountpoint}"
  else
    echo "==> Reusing existing cache volume: ${vol}"
  fi
}

create_volume_if_missing "${VCPKG_VOLUME}"   "/vcpkg-cache"
create_volume_if_missing "${GRADLE_VOLUME}"  "/home/warzone2100/.gradle"
create_volume_if_missing "${CCACHE_VOLUME}"  "/ccache"

echo "==> Running Android ${BUILD_TYPE} build in container..."
docker run --rm \
  --platform linux/amd64 \
  --shm-size=512m \
  -v "${REPO_ROOT}:/src:ro" \
  -v "${OUTPUT_DIR}:/output" \
  -v "${VCPKG_VOLUME}:/vcpkg-cache" \
  -v "${GRADLE_VOLUME}:/home/warzone2100/.gradle" \
  -v "${CCACHE_VOLUME}:/ccache" \
  -e BUILD_TYPE="${BUILD_TYPE}" \
  -e CCACHE_DIR="/ccache" \
  -e CCACHE_MAXSIZE="3G" \
  -e CCACHE_SLOPPINESS="time_macros" \
  "${IMAGE_TAG}" \
  bash /src/docker/android/entrypoint.sh

echo ""
echo "==> Output: ${OUTPUT_DIR}/warzone2100_android_arm64-v8a_$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]').apk"
