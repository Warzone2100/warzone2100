#!/bin/zsh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PLATFORM="${1:-device}"
BUILD_ROOT_BASE="$ROOT_DIR/platforms/ios/build"
BUILD_DIR="${2:-$BUILD_ROOT_BASE/$PLATFORM}"
CONFIGURATION="${IOS_BUILD_CONFIGURATION:-Debug}"
OUTPUT_DIR="${3:-$BUILD_ROOT_BASE/artifacts/$PLATFORM/$CONFIGURATION}"
MAC_APP_PATH="${4:-${WARZONE_DESKTOP_APP_PATH:-/Applications/Warzone 2100.app}}"
VARIANT_LABEL="${IOS_ARTIFACT_VARIANT_LABEL:-${CONFIGURATION:l}}"
if [[ "${CONFIGURATION:l}" == "debug" ]]; then
  if [[ "${IOS_ENABLE_DEBUG_JIT:-0}" == "1" ]]; then
    VARIANT_LABEL="${IOS_ARTIFACT_VARIANT_LABEL:-debug-jit}"
  else
    VARIANT_LABEL="${IOS_ARTIFACT_VARIANT_LABEL:-debug-jitless}"
  fi
fi
REPORT_PATH="${OUTPUT_DIR}/warzone2100-macos-vs-ios-${PLATFORM}-${VARIANT_LABEL}.txt"

is_resource_app() {
  local candidate="$1"
  [[ -d "$candidate/Contents/Resources" || -f "$candidate/Info.plist" ]]
}

if ! is_resource_app "$MAC_APP_PATH"; then
  for candidate in \
    "$HOME/Library/Containers/io.playcover.PlayCover/Applications/com.pumpkinstudios.warzone2100.app" \
    "$HOME/Library/Containers/io.playcover.PlayCover/Applications/com.pumpkinstudiios.warzone2100.app" \
    "$HOME/Applications/PlayCover/Warzone2100.app"; do
    if is_resource_app "$candidate"; then
      MAC_APP_PATH="$candidate"
      break
    fi
  done
fi

case "$PLATFORM" in
  device|simulator)
    ;;
  *)
    echo "usage: $0 [device|simulator] [build-dir] [output-dir] [installed-macos-app]" >&2
    exit 1
    ;;
esac

cmake \
  -DBUILD_ROOT="$BUILD_DIR" \
  -DIOS_PLATFORM="$PLATFORM" \
  -P "$ROOT_DIR/platforms/ios/cmake/configure.cmake"

if [[ "$PLATFORM" == "device" ]]; then
  DESTINATION="generic/platform=iOS"
  APP_GLOB="*${CONFIGURATION}-iphoneos/Warzone2100.app"
else
  DESTINATION="platform=iOS Simulator,name=${IOS_SIMULATOR_NAME:-iPhone 15 Pro},OS=${IOS_SIMULATOR_OS:-17.5}"
  APP_GLOB="*${CONFIGURATION}-iphonesimulator/Warzone2100.app"
fi

xcodebuild \
  -project "$BUILD_DIR/warzone2100.xcodeproj" \
  -scheme warzone2100 \
  -configuration "$CONFIGURATION" \
  -destination "$DESTINATION" \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  build

APP_PATH="$(find "$BUILD_DIR" -type d -path "$APP_GLOB" | head -n 1)"
if [[ -z "$APP_PATH" ]]; then
  echo "failed to locate built Warzone2100.app in $BUILD_DIR" >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
if [[ "$PLATFORM" == "device" ]]; then
  "$ROOT_DIR/platforms/ios/package_ipa.sh" "$APP_PATH" "$OUTPUT_DIR" "$CONFIGURATION" "$MAC_APP_PATH"
else
  "$ROOT_DIR/platforms/ios/stage_app_payload.sh" "$APP_PATH" "$MAC_APP_PATH"
fi
if is_resource_app "$MAC_APP_PATH"; then
  python3 "$ROOT_DIR/platforms/ios/tools/compare_apps.py" \
    --mac-app "$MAC_APP_PATH" \
    --ios-app "$APP_PATH" \
    --output "$REPORT_PATH"
elif [[ "${IOS_ALLOW_SOURCE_PAYLOAD:-0}" == "1" ]]; then
  {
    echo "No installed macOS app was available on this runner."
    echo "The iOS app was packaged with its source-built payload only."
  } > "$REPORT_PATH"
else
  echo "missing installed Warzone app resources: $MAC_APP_PATH" >&2
  exit 1
fi

echo "$APP_PATH"
echo "$REPORT_PATH"
