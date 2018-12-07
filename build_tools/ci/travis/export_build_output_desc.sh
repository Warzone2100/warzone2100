#!/bin/bash

# IMPORTANT:
# This script is meant to be sourced from the root of the working copy.
#
# USAGE:
# Source travis_build_output_desc.sh
#
# Example:
#	source travis_build_output_desc.sh
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Determine the build output description prefix (nightly vs tag/release builds)
# For nightly builds, base on the autorevision info: <BRANCH>-
# For tags/releases, base on the tag name: <TAG_NAME>-
if [ -n "${TRAVIS_TAG}" ] && [ -z "${TRAVIS_PULL_REQUEST_BRANCH}" ]; then
	# Replace "/" so the TRAVIS_TAG can be used in a filename
	TRAVIS_TAG_SANITIZED="$(echo "${TRAVIS_TAG}" | sed -e 's:/:_:' -e 's:-:_:')"
	export WZ_BUILD_DESC_PREFIX="${TRAVIS_TAG_SANITIZED}"
else
	# Collect current working copy Git information
	GIT_BRANCH="$(git branch --no-color | sed -e '/^[^*]/d' -e 's:* \(.*\):\1:')"

	if [ -n "${TRAVIS_PULL_REQUEST_BRANCH}" ]; then
		echo "Triggered by a Pull Request - use the TRAVIS_PULL_REQUEST_BRANCH (${TRAVIS_PULL_REQUEST_BRANCH}) as the branch for the output filename"
		GIT_BRANCH="${TRAVIS_PULL_REQUEST_BRANCH}"
	elif [ -n "${TRAVIS_BRANCH}" ]; then
		echo "Use the TRAVIS_BRANCH (${TRAVIS_BRANCH}) as the branch for the output filename"
		GIT_BRANCH="${TRAVIS_BRANCH}"
	fi

	# Replace "/" so the GIT_BRANCH can be used in a filename
	GIT_BRANCH_SANITIZED="$(echo "${GIT_BRANCH}" | sed -e 's:/:_:' -e 's:-:_:')"
	export WZ_BUILD_DESC_PREFIX="${GIT_BRANCH_SANITIZED}"
fi

echo "WZ_BUILD_DESC_PREFIX=${WZ_BUILD_DESC_PREFIX}"
