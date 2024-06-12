#!/bin/sh

# Expected command-line args:
# - Path to each architecture WZ app bundle
#
# This script lipo-s together all of the main executables into a universal binary
# It uses the first WZ app bundle path as the "base" app bundle, and then replaces its main executable with the universal binary
#
# This should likely be followed by re-code-signing (as desired)

set -e

WZ_APP_EXE_PATHS=()
WZ_APP_DSYM_PATHS=()

PROJECT_NAME="Warzone 2100"

if [ "$#" -le 1 ]; then
  echo "Requires 2 or more arguments"
  exit 1
fi

for WZ_APP_PATH in "$@" ; do
  if [ ! -d "${WZ_APP_PATH}" ]; then
    echo "\"${WZ_APP_PATH}\" is not a directory"
    exit 1
  fi

  WZ_APP_EXECUTABLE_PATH="${WZ_APP_PATH}/Contents/MacOS/${PROJECT_NAME}"
  if [ ! -f "${WZ_APP_EXECUTABLE_PATH}" ]; then
    echo "\"${WZ_APP_EXECUTABLE_PATH}\" does not exist"
    exit 1
  fi
  WZ_APP_EXE_PATHS+=("${WZ_APP_EXECUTABLE_PATH}")

  WZ_APP_DSYM_PATH="${WZ_APP_PATH}/Contents/MacOS/${PROJECT_NAME}.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}"
  if [ -f "${WZ_APP_DSYM_PATH}" ]; then
    WZ_APP_DSYM_PATHS+=("${WZ_APP_DSYM_PATH}")
  else
    echo "Missing expected dSYM path: ${WZ_APP_DSYM_PATH}"
  fi
done

echo "WZ_APP_EXE_PATHS=${WZ_APP_EXE_PATHS[@]}"

# lipo together all of the single-arch binaries
echo "lipo -create -output \"${PROJECT_NAME}-universal\" ${WZ_APP_EXE_PATHS[@]}"
lipo -create -output "${PROJECT_NAME}-universal" "${WZ_APP_EXE_PATHS[@]}"

# lipo together all of the dSYMs
if [ "${#WZ_APP_DSYM_PATHS[@]}" -gt 0 ]; then
  mkdir -p "${PROJECT_NAME}-universal.dSYM/Contents/Resources/DWARF/"
  echo "lipo -create -output \"${PROJECT_NAME}-universal.dSYM\" ${WZ_APP_DSYM_PATHS[@]}"
  lipo -create -output "${PROJECT_NAME}-universal.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}" "${WZ_APP_DSYM_PATHS[@]}"
fi

# Replace the first app bundle's single-arch binary with the universal binary
BASE_WZ_APP_PATH="$1"
BASE_WZ_APP_EXECUTABLE_PATH="${BASE_WZ_APP_PATH}/Contents/MacOS/${PROJECT_NAME}"
rm "${BASE_WZ_APP_EXECUTABLE_PATH}"
echo "mv \"${PROJECT_NAME}-universal\" \"${BASE_WZ_APP_EXECUTABLE_PATH}\""
mv "${PROJECT_NAME}-universal" "${BASE_WZ_APP_EXECUTABLE_PATH}"

# Replace the first app bundle's dSYM with the universal dSYM
if [ "${#WZ_APP_DSYM_PATHS[@]}" -gt 0 ]; then
  BASE_WZ_APP_DSYM_PATH="${BASE_WZ_APP_PATH}/Contents/MacOS/${PROJECT_NAME}.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}"
  rm "${BASE_WZ_APP_DSYM_PATH}"
  echo "mv universal dSYM"
  mv "${PROJECT_NAME}-universal.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}" "${BASE_WZ_APP_DSYM_PATH}"
  rm -rf "${PROJECT_NAME}-universal.dSYM/"
fi

echo "Done."
