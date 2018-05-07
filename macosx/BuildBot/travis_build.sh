#!/bin/bash

# IMPORTANT:
# This script is meant to be run from the root of the working copy.
#
# USAGE:
# Execute travis_build.sh with 1-2 parameters
# 1.) Specify one of the valid modes: ("regular", "release") REQUIRED
# 2.) Specify an output path for the created warzone2100*_macOS.zip files (optional, default: macosx/build/wz_output)
#
# Example:
#	./travis_build.sh nightly "tmp/build_output"
#
# Regular builds:
# - Warzone.zip containing:
#		- Warzone.app built in Release mode with debugging symbols included, but *NO* video sequences
#
# Release builds:
# - Warzone.zip containing:
#		- Warzone.app built in Release mode with debugging symbols included, *AND* video sequences
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Future TODO: Use TRAVIS_TAG to detect if the build is for a git tag, and parse the
#              tag to see if it's likely a release (to automatically set MODE="release")

# Handle arguments
if [ -z "$1" ]; then
	echo "travis_build.sh requires an argument specifying one of the valid modes: (\"regular\", \"release\")"
	exit 1
fi
if ! [[ "$1" =~ ^(regular|release)$ ]]; then
	echo "travis_build.sh requires an argument specifying one of the valid modes: (\"regular\", \"release\")"
	exit 1
fi

BUILD_MODE="$1"
OUTPUT_DIR="macosx/build/wz_output"
if [ -n "$2" ]; then
	OUTPUT_DIR="$2"
fi
if [ ! -d "${OUTPUT_DIR}" ]; then
	mkdir -p "${OUTPUT_DIR}"
fi

# Determine whether to fetch & include the videos in the built app bundle
function shouldIncludeVideoSequences {
	local MODE="$1"

	if [ -n "${TRAVIS_PULL_REQUEST}" ] && [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
		# Skip downloading videos for pull requests
		echo "Skip including video sequences for pull requests."
		return 255
	fi
	if [ "${MODE}" != "release" ]; then
		# Skip downloading videos for non-release commits
		echo "Skip including video sequences because input mode is not set to \"release\"."
		return 255
	fi

	true
}

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

# Clean
echo "macosx/BuildBot/00_clean.sh"
macosx/BuildBot/00_clean.sh
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: 00_clean.sh failed"
	exit ${result}
fi
find "${OUTPUT_DIR}" -name "warzone2100-*.zip" -exec rm -r "{}" \;

# Configure
echo "macosx/BuildBot/01_configure.sh"
macosx/BuildBot/01_configure.sh
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: 01_configure.sh failed"
	exit ${result}
fi

# Fetch the video sequences (if configured)
if shouldIncludeVideoSequences "${BUILD_MODE}"; then
	echo "Fetching video sequences..."
	echo "macosx/BuildBot/02_fetchvideosequences.sh"
	macosx/BuildBot/02_fetchvideosequences.sh
	result=${?}
	if [ $result -ne 0 ]; then
		echo "ERROR: 02_fetchvideosequences.sh failed"
		exit ${result}
	fi
fi

# Future TODO: Retrieve the signing certificate from Travis secure data, and configure code-signing

# Compile Warzone.app (and Warzone.zip)
echo "macosx/BuildBot/03_compile.sh"
macosx/BuildBot/03_compile.sh
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: 03_compile.sh failed"
	exit ${result}
fi

# Verify Warzone.zip was created
BUILT_WARZONE_ZIP="macosx/build/Release/Warzone.zip"
if [ ! -f "${BUILT_WARZONE_ZIP}" ]; then
	echo "ERROR: Something went wrong, and Warzone.zip does not exist"
	exit 1
fi

# Collect current working copy Git information
GIT_BRANCH="$(git branch --no-color | sed -e '/^[^*]/d' -e 's:* \(.*\):\1:')"
BUILT_DATETIME=$(date '+%Y%m%d-%H%M%S')
GIT_REVISION_SHORT="$(git rev-parse -q --short --verify HEAD | cut -c1-7)"

if [ -n "${TRAVIS_PULL_REQUEST_BRANCH}" ]; then
	echo "Triggered by a Pull Request - use the TRAVIS_PULL_REQUEST_BRANCH (${TRAVIS_PULL_REQUEST_BRANCH}) as the branch for the output filename"
	GIT_BRANCH="${TRAVIS_PULL_REQUEST_BRANCH}"
elif [ -n "${TRAVIS_BRANCH}" ]; then
	echo "Use the TRAVIS_BRANCH (${TRAVIS_BRANCH}) as the branch for the output filename"
	GIT_BRANCH="${TRAVIS_BRANCH}"
fi

# Replace "/" so the GIT_BRANCH can be used in a filename
GIT_BRANCH_SANITIZED="$(echo "${GIT_BRANCH}" | sed -e 's:/:_:')"

# Move Warzone.zip to the output directory, renaming it to:
#  warzone2100-{GIT_BRANCH_SANITIZED}-{BUILT_DATETIME}-{GIT_REVISION_SHORT}_macOS.zip
DESIRED_ZIP_NAME="warzone2100-${GIT_BRANCH_SANITIZED}-${BUILT_DATETIME}-${GIT_REVISION_SHORT}_macOS.zip"
mv "$BUILT_WARZONE_ZIP" "${OUTPUT_DIR}/${DESIRED_ZIP_NAME}"
result=${?}
if [ $result -ne 0 ]; then
	echo "ERROR: Failed to move zip file"
	exit ${result}
fi
echo "Generated Warzone.zip: \"${OUTPUT_DIR}/${DESIRED_ZIP_NAME}\""

exit 0
