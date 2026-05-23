#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_TYPE_LOWER="${BUILD_TYPE,,}"

echo "==> Build type: ${BUILD_TYPE}"
echo "==> ANDROID_NDK_HOME: ${ANDROID_NDK_HOME}"
echo "==> VCPKG_ROOT: ${VCPKG_ROOT}"

# Force ANDROID_NDK to match ANDROID_NDK_HOME so the CMake toolchain doesn't
# pick up a stale r27 path that may exist in the Docker layer cache.
export ANDROID_NDK="${ANDROID_NDK_HOME}"
echo "==> ANDROID_NDK: ${ANDROID_NDK}"

# /src is the mounted repo; /build is a writable working copy
# Copy source to writable dir to avoid polluting the mounted repo
WORK_DIR="/home/warzone2100/work"
mkdir -p "${WORK_DIR}"
echo "==> Copying source to ${WORK_DIR}..."
cp -a /src/. "${WORK_DIR}/"
cd "${WORK_DIR}"

# Init submodules
echo "==> Initializing git submodules..."
git submodule update --init --recursive

# vcpkg install into a cacheable directory
VCPKG_INSTALLED="/vcpkg-cache/installed"
mkdir -p "${VCPKG_INSTALLED}"

echo "==> Installing vcpkg dependencies (arm64-android)..."
# Limit ninja parallelism to avoid OOM (exit 137) during heavy packages like harfbuzz
export VCPKG_MAX_CONCURRENCY=2
export VCPKG_KEEP_ENV_VARS="CMAKE_BUILD_PARALLEL_LEVEL"
export CMAKE_BUILD_PARALLEL_LEVEL=2
"${VCPKG_ROOT}/vcpkg" install \
  --triplet arm64-android \
  --x-install-root="${VCPKG_INSTALLED}" \
  --overlay-ports="${WORK_DIR}/.ci/vcpkg/overlay-ports" || {
    echo ""
    echo "==> vcpkg install failed — dumping buildtree logs:"
    find "${VCPKG_ROOT}/buildtrees" \( -name "*-out.log" -o -name "*-err.log" \) -print \
      | sort | while IFS= read -r log; do
        echo ""
        echo "========== ${log} =========="
        cat "${log}"
      done
    exit 1
  }

# Create game data archives for APK assets
# WZActivity.java copies these to internal storage; PhysFS mounts them from there.
WZ_ASSETS_DIR="${WORK_DIR}/platforms/android/app/src/main/assets/data"
mkdir -p "${WZ_ASSETS_DIR}"
echo "==> Creating base.wz from data/base ..."
(cd "${WORK_DIR}/data/base" && zip -r "${WZ_ASSETS_DIR}/base.wz" . -x "*.DS_Store")

# Append fonts to base.wz under fonts/ so PhysFS finds fonts/DejaVuSans.ttf
echo "==> Appending fonts to base.wz ..."
FONTS_TMPDIR=$(mktemp -d)
mkdir -p "${FONTS_TMPDIR}/fonts"
for _f in DejaVuSans.ttf DejaVuSans-Bold.ttf DejaVu.LICENSE.txt; do
    _src="${WORK_DIR}/data/fonts/DejaVu/${_f}"
    [ -f "${_src}" ] && cp "${_src}" "${FONTS_TMPDIR}/fonts/"
done
(cd "${FONTS_TMPDIR}" && zip -r "${WZ_ASSETS_DIR}/base.wz" fonts/)
rm -rf "${FONTS_TMPDIR}"

echo "==> Creating mp.wz from data/mp ..."
(cd "${WORK_DIR}/data/mp"   && zip -r "${WZ_ASSETS_DIR}/mp.wz"   . -x "*.DS_Store")
echo "==> Game data archives staged in ${WZ_ASSETS_DIR}"

# Stage SDL3 shared library required by Android Gradle build
SDL3_SO="${VCPKG_INSTALLED}/arm64-android/lib/libSDL3.so"
JNI_DIR="${WORK_DIR}/platforms/android/app/src/main/jniLibs/arm64-v8a"
mkdir -p "${JNI_DIR}"
if [ -f "${SDL3_SO}" ]; then
  cp "${SDL3_SO}" "${JNI_DIR}/libSDL3.so"
  echo "==> Staged libSDL3.so"
else
  echo "WARNING: libSDL3.so not found at ${SDL3_SO} — build may fail"
fi

# Generate Gradle wrapper JAR (not stored in git)
cd "${WORK_DIR}/platforms/android"
chmod +x gradlew
gradle wrapper --gradle-version 8.11.1 --distribution-type bin

# Build APK
echo "==> Running gradlew assemble${BUILD_TYPE}..."
./gradlew "assemble${BUILD_TYPE}" \
  -Pandroid.ndkPath="${ANDROID_NDK_HOME}" \
  -Pvcpkg.installed.dir="${VCPKG_INSTALLED}" \
  --no-daemon \
  --stacktrace

# Copy APK to output volume
APK_PATH=$(find . -name "*.apk" -path "*/${BUILD_TYPE_LOWER}/*" | head -1)
if [ -z "${APK_PATH}" ]; then
  echo "ERROR: No APK found under platforms/android after build"
  exit 1
fi

OUTPUT_NAME="warzone2100_android_arm64-v8a_${BUILD_TYPE_LOWER}.apk"
cp "${APK_PATH}" "/output/${OUTPUT_NAME}"
echo ""
echo "==> APK: /output/${OUTPUT_NAME}"
echo "==> SHA512: $(sha512sum "/output/${OUTPUT_NAME}")"
echo "==> Size: $(stat -c %s "/output/${OUTPUT_NAME}") bytes"
