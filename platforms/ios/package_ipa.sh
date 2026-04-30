#!/bin/zsh
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <ios-app-path> <output-dir> [configuration] [installed-macos-app]" >&2
  exit 1
fi

APP_PATH="$1"
OUTPUT_DIR="$2"
CONFIGURATION="${3:-Release}"
MAC_APP_PATH="${4:-/Applications/Warzone 2100.app}"
BUNDLE_ID="com.pumpkinstudios.warzone2100"
VERSION_LABEL="0.1-beta"
SCRIPT_DIR="$(dirname "$0")"

case "${CONFIGURATION:l}" in
  debug)
    VERSION_LABEL="${VERSION_LABEL}-debug-regular"
    ;;
  relwithdebinfo)
    VERSION_LABEL="${VERSION_LABEL}-relwithdebinfo"
    ;;
esac

if [[ ! -d "$APP_PATH" ]]; then
  echo "missing app bundle: $APP_PATH" >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
"$SCRIPT_DIR/stage_app_payload.sh" "$APP_PATH" "$MAC_APP_PATH"

EXECUTABLE_NAME="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP_PATH/Info.plist")"
EXECUTABLE_PATH="$APP_PATH/$EXECUTABLE_NAME"
if [[ ! -f "$EXECUTABLE_PATH" ]]; then
  echo "missing executable inside app bundle: $EXECUTABLE_PATH" >&2
  exit 1
fi

LDID_BIN="${LDID:-}"
ENTITLEMENTS_PATH="$SCRIPT_DIR/Resources/Warzone-iOS.entitlements"
JIT_ENTITLEMENTS_PATH="$SCRIPT_DIR/Resources/Warzone-iOS-DebugJIT.entitlements"
if [[ -z "$LDID_BIN" ]]; then
  if command -v ldid >/dev/null 2>&1; then
    LDID_BIN="$(command -v ldid)"
  elif [[ -x /opt/homebrew/bin/ldid ]]; then
    LDID_BIN="/opt/homebrew/bin/ldid"
  fi
fi
if [[ -z "$LDID_BIN" || ! -x "$LDID_BIN" ]]; then
  echo "missing ldid; install it or set LDID=/path/to/ldid" >&2
  exit 1
fi

/usr/libexec/PlistBuddy -c "Set :TSBundlePreSigned YES" "$APP_PATH/Info.plist" 2>/dev/null \
  || /usr/libexec/PlistBuddy -c "Add :TSBundlePreSigned bool YES" "$APP_PATH/Info.plist"

package_variant() {
  local label="$1"
  local entitlements="$2"
  if [[ -f "$entitlements" ]]; then
    "$LDID_BIN" "-S$entitlements" "$EXECUTABLE_PATH"
  else
    "$LDID_BIN" -S "$EXECUTABLE_PATH"
  fi

  local staging_dir
  staging_dir="$(mktemp -d)"
  mkdir -p "$staging_dir/Payload"
  cp -R "$APP_PATH" "$staging_dir/Payload/"
  find "$staging_dir" -name '.DS_Store' -delete
  /usr/bin/xattr -cr "$staging_dir" || true

  local ipa_path="$OUTPUT_DIR/${BUNDLE_ID}_${label}.ipa"
  rm -f "$ipa_path"
  (cd "$staging_dir" && COPYFILE_DISABLE=1 /usr/bin/zip -Xqry "$ipa_path" Payload)
  rm -rf "$staging_dir"

  echo "$ipa_path"
}

package_variant "$VERSION_LABEL" "$ENTITLEMENTS_PATH"

if [[ "${CONFIGURATION:l}" == "debug" ]]; then
  package_variant "0.1-beta-debug-jit" "$JIT_ENTITLEMENTS_PATH"
fi
