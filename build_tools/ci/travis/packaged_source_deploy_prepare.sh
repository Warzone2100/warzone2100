#!/bin/bash

# IMPORTANT:
# This script is meant to be run from the root of the working copy.
#
# USAGE:
# Execute packaged_source_deploy_prepare.sh with 3 parameters
# 1.) Specify an input file
# 2.) Specify a build output description prefix (ex. "master-") REQUIRED
# 3.) Specify an output directory for the copied (and automatically-named) file REQUIRED
#
# Example:
#  build_tools/ci/travis/packaged_source_deploy_prepare.sh "/build/warzone2100.tar.xz" "${WZ_BUILD_DESC_PREFIX}-" "tmp/wz_upload"
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )

if [ -z "$1" ]; then
	echo "packaged_source_deploy_prepare.sh requires an argument specifying an input file"
	exit 1
fi
if [ ! -f "$1" ]; then
	echo "packaged_source_deploy_prepare.sh requires an argument specifying an input file that exists (does not exist: $1)"
	exit 1
fi
INPUT_FILE="$1"

if [ -z "$2" ]; then
	echo "packaged_source_deploy_prepare.sh requires an argument specifying a build output description prefix"
	exit 1
fi
BUILD_OUTPUT_DESCRIPTION_PREFIX="$2"

if [ -z "$3" ]; then
	echo "packaged_source_deploy_prepare.sh requires an argument specifying an output directory"
	exit 1
fi
OUTPUT_DIR="$3"
if [ ! -d "${OUTPUT_DIR}" ]; then
	mkdir -p "${OUTPUT_DIR}"
fi

BUILT_DATETIME=$(date '+%Y%m%d-%H%M%S')
GIT_REVISION_SHORT="$(git rev-parse -q --short --verify HEAD | cut -c1-7)"
WZ_BUILD_DESC="${BUILD_OUTPUT_DESCRIPTION_PREFIX}${BUILT_DATETIME}-${GIT_REVISION_SHORT}"

# Copy & rename the input file
cp "${INPUT_FILE}" "${OUTPUT_DIR}/warzone2100-${WZ_BUILD_DESC}_src.tar.xz"

# Output the SHA512 + size of the .tar.xz
echo "Generated warzone2100 tarball: \"warzone2100-${WZ_BUILD_DESC}_src.tar.xz\""
echo "  -> SHA512: $(sha512sum "${OUTPUT_DIR}/warzone2100-${WZ_BUILD_DESC}_src.tar.xz")"
echo "  -> Size (bytes): $(stat -c %s "${OUTPUT_DIR}/warzone2100-${WZ_BUILD_DESC}_src.tar.xz")"

exit 0

