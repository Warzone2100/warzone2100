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

mkdir -p "${OUTPUT_DIR}"

echo "==> Building Docker image: ${IMAGE_TAG}"
docker build \
  --platform linux/amd64 \
  --build-arg UID="$(id -u)" \
  -f "${SCRIPT_DIR}/Dockerfile" \
  -t "${IMAGE_TAG}" \
  "${REPO_ROOT}"

echo "==> Creating/Recreating vcpkg cache volume: ${VCPKG_VOLUME}"
docker volume rm wz2100-vcpkg-android-cache > /dev/null 2>&1 || true
docker volume create "${VCPKG_VOLUME}" > /dev/null

echo "==> Fixing vcpkg cache volume permissions..."
docker run --rm --user root \
  --platform linux/amd64 \
  -v "${VCPKG_VOLUME}:/vcpkg-cache" \
  "${IMAGE_TAG}" \
  chown -R warzone2100:root /vcpkg-cache

echo "==> Running Android ${BUILD_TYPE} build in container..."
docker run --rm \
  --platform linux/amd64 \
  --shm-size=512m \
  -v "${REPO_ROOT}:/src:ro" \
  -v "${OUTPUT_DIR}:/output" \
  -v "${VCPKG_VOLUME}:/vcpkg-cache" \
  -e BUILD_TYPE="${BUILD_TYPE}" \
  "${IMAGE_TAG}" \
  bash /src/docker/android/entrypoint.sh

echo ""
echo "==> Output: ${OUTPUT_DIR}/warzone2100_android_arm64-v8a_$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]').apk"
