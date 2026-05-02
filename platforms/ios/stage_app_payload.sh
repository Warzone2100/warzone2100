#!/bin/zsh
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <ios-app-path> [installed-macos-app]" >&2
  exit 1
fi

APP_PATH="$1"
MAC_APP_PATH="${2:-${WARZONE_DESKTOP_APP_PATH:-/Applications/Warzone 2100.app}}"

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

if [[ ! -d "$APP_PATH" ]]; then
  echo "missing app bundle: $APP_PATH" >&2
  exit 1
fi
if [[ -d "$MAC_APP_PATH/Contents/Resources" ]]; then
  RESOURCE_ROOT="$MAC_APP_PATH/Contents/Resources"
  SOURCE_LAYOUT="macos"
elif [[ -f "$MAC_APP_PATH/Info.plist" ]]; then
  RESOURCE_ROOT="$MAC_APP_PATH"
  SOURCE_LAYOUT="flat"
else
  if [[ "${IOS_ALLOW_SOURCE_PAYLOAD:-0}" == "1" ]]; then
    echo "missing installed Warzone app resources; keeping source-built iOS payload"
    exit 0
  fi
  echo "missing installed Warzone app resources: $MAC_APP_PATH" >&2
  exit 1
fi

SOURCE_EXECUTABLE="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$MAC_APP_PATH/Info.plist" 2>/dev/null || true)"

for entry_path in "$RESOURCE_ROOT"/*; do
  entry_name="${entry_path:t}"
  if [[ ! -e "$entry_path" ]]; then
    continue
  fi
  if [[ "$SOURCE_LAYOUT" == "flat" ]]; then
    case "$entry_name" in
      "$SOURCE_EXECUTABLE"|Info.plist|PkgInfo|_CodeSignature|PlugIns|Frameworks)
        continue
        ;;
    esac
  fi
  rm -rf "$APP_PATH/$entry_name"
  cp -R "$entry_path" "$APP_PATH/$entry_name"
done

if [[ -f "$APP_PATH/Warzone.icns" ]]; then
  rm -f "$APP_PATH/Warzone2100.icns"
  mv "$APP_PATH/Warzone.icns" "$APP_PATH/Warzone2100.icns"
fi

find "$APP_PATH" -name '.DS_Store' -delete
/usr/bin/xattr -cr "$APP_PATH" || true
