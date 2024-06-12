#!/bin/sh

# Expects the following input parameter:
# 1. The input warzone 2100 zip
# 2. The temporary extraction directory (optional)

# Verify "warzone2100.zip" was created
BUILT_WARZONE_ZIP="$1"
if [ ! -f "${BUILT_WARZONE_ZIP}" ]; then
  echo "ERROR: Something went wrong, and \"${BUILT_WARZONE_ZIP}\" does not exist"
  exit 1
fi
TMP_PKG_EXTRACT_DIR="build/tmp/_wzextract"
if [ ! -n "$2" ]; then
  TMP_PKG_EXTRACT_DIR="$2/_wzextract"
fi

# Extract & verify the .zip contents
rm -rf "${TMP_PKG_EXTRACT_DIR}"
if [ ! -d "${TMP_PKG_EXTRACT_DIR}" ]; then
  mkdir -p "${TMP_PKG_EXTRACT_DIR}"
fi
unzip -qq "${BUILT_WARZONE_ZIP}" -d "${TMP_PKG_EXTRACT_DIR}"
cd "${TMP_PKG_EXTRACT_DIR}"
if [ ! -d "Warzone 2100.app" ]; then
  echo "ERROR: \"Warzone 2100.app\" was not extracted from \"${BUILT_WARZONE_ZIP}\""
  exit 1
fi
# For debugging purposes, output some information about the generated "Warzone 2100.app" (inside the .zip)
echo "Generated \"Warzone 2100.app\""
generated_infoplist_location="Warzone 2100.app/Contents/Info.plist"
generated_executable_location="Warzone 2100.app/Contents/MacOS/Warzone 2100"
generated_versionnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "${generated_infoplist_location}")
echo "  -> Version Number (CFBundleShortVersionString): ${generated_versionnumber}"
generated_buildnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "${generated_infoplist_location}")
echo "  -> Build Number (CFBundleVersion): ${generated_buildnumber}"
generated_minimumsystemversion=$(/usr/libexec/PlistBuddy -c "Print LSMinimumSystemVersion" "${generated_infoplist_location}")
echo "  -> Minimum macOS (LSMinimumSystemVersion): ${generated_minimumsystemversion}"
generated_exe_arch="$(lipo -info "${generated_executable_location}")"
echo "  -> lipo -info \"${generated_executable_location}\""
echo "     ${generated_exe_arch}"
codesign_verify_result=$(codesign --verify --deep --strict --verbose=2 "Warzone 2100.app" 2>&1)
echo "  -> codesign --verify --deep --strict --verbose=2 \"Warzone 2100.app\""
if [ -n "${codesign_verify_result}" ]; then
  while read -r line; do
    echo "     $line"
  done <<< "$codesign_verify_result"
else
  echo "     (No output?)"
fi
cd - > /dev/null

# Remove the temporary extracted copy
rm -rf "${TMP_PKG_EXTRACT_DIR}"
