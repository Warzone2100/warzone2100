#!/bin/bash

# IMPORTANT:
# This script is meant to be run from the root of the working copy.
#
# USAGE:
# Execute travis_build.sh with 1-2 parameters
# 1.) Specify a build output description prefix (ex. "master-") REQUIRED
# 2.) Specify an output path for the created warzone2100-${BUILD_OUTPUT_DESCRIPTION_PREFIX}-*_macOS.zip files (optional, default: macosx/build/wz_output)
#
# Example:
#  source build_tools/ci/travis/export_build_output_desc.sh
#  cd build && macosx/BuildBot/travis_cmake_build.sh "${WZ_BUILD_DESC_PREFIX}-" "tmp/build_output"
#
# Output:
# - "warzone2100-${BUILD_OUTPUT_DESCRIPTION_PREFIX}-*_macOS.zip" containing:
#		- "Warzone 2100.app" built in Release mode with debugging symbols included, but *NO* video sequences
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Security:
#   Unset Travis-CI secure environment variables, so they are not passed to any of the subsequent build scripts / commands
unset SECURE_UPLOAD_BASE64_KEY

# Handle arguments
if [ -z "$1" ]; then
	echo "travis_build.sh requires an argument specifying a build output description prefix"
	exit 1
fi
BUILD_OUTPUT_DESCRIPTION_PREFIX="$1"

OUTPUT_DIR="macosx/build/wz_output"
if [ -n "$2" ]; then
	OUTPUT_DIR="$2"
fi
if [ ! -d "${OUTPUT_DIR}" ]; then
	mkdir -p "${OUTPUT_DIR}"
fi

# Travis-CI: Repo prep
# (Travis limits Git clone depth, but we need the full history *and* the master branch)

# See: https://stackoverflow.com/a/44036486
function create_all_branches()
{
	# Keep track of where Travis put us.
	# We are on a detached head, and we need to be able to go back to it.
	local build_head=$(git rev-parse HEAD)

	# Fetch all the remote branches. Travis clones with `--depth`, which
	# implies `--single-branch`, so we need to overwrite remote.origin.fetch to
	# do that.
	echo "git config --replace-all remote.origin.fetch +refs/heads/*:refs/remotes/origin/*"
	git config --replace-all remote.origin.fetch +refs/heads/*:refs/remotes/origin/*
	echo "travis_fold:start:git.fetch.unshallow"
	echo "git fetch --unshallow"
	git fetch --unshallow
	echo "travis_fold:end:git.fetch.unshallow"
	# also fetch the tags
	echo "travis_fold:start:git.fetch.tags"
	echo "git fetch --tags"
	git fetch --tags
	echo "travis_fold:end:git.fetch.tags"

	# checkout master to ensure that a local master branch exists
	echo "git checkout -qf master"
	git checkout -qf master

	# finally, go back to where we were at the beginning
	echo "git checkout -qf ${build_head}"
	git checkout -qf ${build_head}
}

echo "travis_fold:start:travis.repo.prep"
echo "Travis cloned repo prep..."
create_all_branches
echo "Finished preparing cloned repo."
echo "travis_fold:end:travis.repo.prep"

# Get dependencies
echo "travis_fold:start:wz.get.dependencies.mac"
echo "./get-dependencies_mac.sh release -autofix"
./get-dependencies_mac.sh release -autofix
result=${?}
echo "travis_fold:end:wz.get.dependencies.mac"
if [ $result -ne 0 ]; then
	echo "ERROR: get-dependencies_mac.sh failed"
	exit ${result}
fi

# Future TODO: Retrieve the signing certificate from Travis secure data, and configure code-signing

# Delete any prior build dir
rm -rf build

# CMake configure
echo "travis_fold:start:wz.cmake.configure"
WZ_DISTRIBUTOR="UNKNOWN"
if [ "${TRAVIS_REPO_SLUG}" == "Warzone2100/warzone2100" ]; then
	# Building from main repo - set distributor
	WZ_DISTRIBUTOR="wz2100.net"
fi
echo "cmake '-H.' -Bbuild -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DWZ_DISTRIBUTOR:STRING=\"${WZ_DISTRIBUTOR}\" -G\"Xcode\""
cmake '-H.' -Bbuild -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DWZ_DISTRIBUTOR:STRING="${WZ_DISTRIBUTOR}" -G"Xcode"
result=${?}
echo "travis_fold:end:wz.cmake.configure"
if [ $result -ne 0 ]; then
	echo "ERROR: CMake configure failed"
	exit ${result}
fi

# Include Xcode helpers
. macosx/BuildBot/_xcodebuild_helpers.sh

# Compile "Warzone 2100.app", and package "warzone2100.zip"
execute_xcodebuild_command  \
 -project build/warzone2100.xcodeproj \
 -target "package" \
 -configuration "Release" \
 -destination "platform=macOS" \
 -PBXBuildsContinueAfterErrors=NO
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: xcodebuild failed"
	exit ${result}
fi

# Verify "warzone2100.zip" was created
BUILT_WARZONE_ZIP="build/warzone2100.zip"
if [ ! -f "${BUILT_WARZONE_ZIP}" ]; then
	echo "ERROR: Something went wrong, and \"${BUILT_WARZONE_ZIP}\" does not exist"
	exit 1
fi

# Extract & verify the .zip contents
TMP_PKG_EXTRACT_DIR="build/tmp/_wzextract"
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
generated_versionnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "${generated_infoplist_location}")
echo "  -> Version Number (CFBundleShortVersionString): ${generated_versionnumber}"
generated_buildnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "${generated_infoplist_location}")
echo "  -> Build Number (CFBundleVersion): ${generated_buildnumber}"
generated_minimumsystemversion=$(/usr/libexec/PlistBuddy -c "Print LSMinimumSystemVersion" "${generated_infoplist_location}")
echo "  -> Minimum macOS (LSMinimumSystemVersion): ${generated_minimumsystemversion}"
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

# Determine the build output description
# <BUILD_OUTPUT_DESCRIPTION_PREFIX><BUILT_DATETIME>-<GIT_REVISION_SHORT>
BUILT_DATETIME=$(date '+%Y%m%d-%H%M%S')
GIT_REVISION_SHORT="$(git rev-parse -q --short --verify HEAD | cut -c1-7)"
WZ_BUILD_DESC="${BUILD_OUTPUT_DESCRIPTION_PREFIX}${BUILT_DATETIME}-${GIT_REVISION_SHORT}"

# Move warzone2100.zip to the output directory, renaming it
DESIRED_ZIP_NAME="warzone2100-${WZ_BUILD_DESC}_macOS.zip"
mv "$BUILT_WARZONE_ZIP" "${OUTPUT_DIR}/${DESIRED_ZIP_NAME}"
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: Failed to move zip file"
	exit ${result}
fi
echo "Generated warzone2100.zip: \"${OUTPUT_DIR}/${DESIRED_ZIP_NAME}\""
ZIP_HASH="$(shasum -a 512 "${OUTPUT_DIR}/${DESIRED_ZIP_NAME}")"
ZIP_SIZE="$(stat -f '%z' "${OUTPUT_DIR}/${DESIRED_ZIP_NAME}")"
echo "  -> SHA512: ${ZIP_HASH}"
echo "  -> Size (bytes): ${ZIP_SIZE}"

exit 0
