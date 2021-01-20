#!/bin/bash

# Get Snapcraft build-time target arch

echoerr() { echo "$@" 1>&2; }

BUILDTIME_TARGET_ARCH=""
if [ -n "${SNAPCRAFT_TARGET_ARCH}" ]; then
  # SNAPCRAFT_TARGET_ARCH is available - use it!
  BUILDTIME_TARGET_ARCH="${SNAPCRAFT_TARGET_ARCH}"
else
  # If SNAPCRAFT_TARGET_ARCH is not available, parse the SNAPCRAFT_ARCH_TRIPLET and convert it
  case ${SNAPCRAFT_ARCH_TRIPLET%%-*} in
  x86_64)
      BUILDTIME_TARGET_ARCH="amd64"
      ;;
  i386)
      BUILDTIME_TARGET_ARCH="i386"
      ;;
  aarch64)
      BUILDTIME_TARGET_ARCH="arm64"
      ;;
  *)
      BUILDTIME_TARGET_ARCH=""
  esac
fi

echo "${BUILDTIME_TARGET_ARCH}"
