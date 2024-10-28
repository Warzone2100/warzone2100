#!/bin/bash

# Get Snapcraft build-time target arch

echoerr() { echo "$@" 1>&2; }

BUILDTIME_TARGET_ARCH=""
if [ -n "${CRAFT_ARCH_BUILD_FOR}" ]; then
  # CRAFT_ARCH_BUILD_FOR is available - use it!
  BUILDTIME_TARGET_ARCH="${CRAFT_ARCH_BUILD_FOR}"
elif [ -n "${CRAFT_TARGET_ARCH}" ]; then
  # CRAFT_TARGET_ARCH is available - use it!
  BUILDTIME_TARGET_ARCH="${CRAFT_TARGET_ARCH}"
else
  # If above are not available, parse the CRAFT_ARCH_TRIPLET_BUILD_FOR and convert it
  case ${CRAFT_ARCH_TRIPLET_BUILD_FOR%%-*} in
  x86_64)
      BUILDTIME_TARGET_ARCH="amd64"
      ;;
  i386)
      BUILDTIME_TARGET_ARCH="i386"
      ;;
  aarch64)
      BUILDTIME_TARGET_ARCH="arm64"
      ;;
  s390x)
      BUILDTIME_TARGET_ARCH="s390x"
      ;;
  powerpc64le)
      BUILDTIME_TARGET_ARCH="ppc64el"
      ;;
  *)
      BUILDTIME_TARGET_ARCH=""
  esac
fi

echo "${BUILDTIME_TARGET_ARCH}"
