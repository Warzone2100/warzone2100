#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script runs autorevision, and tries to download the external libs.
# It attempts the download twice because they may not download properly the first time.
#

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
#

cd macosx

# Run autorevision
execute_xcodebuild_command -project Warzone.xcodeproj -target "Autorevision" -configuration "Release"
result=${?}
if [ $result -ne 0 ]; then
	exit ${result}
fi

# Output the values determined by autorevision
. "../src/autorevision.cache"
echo "Generated autorevision.cache info..."
echo "VCS_TYPE=${VCS_TYPE}"
echo "VCS_BASENAME=${VCS_BASENAME}"
echo "VCS_BRANCH=${VCS_BRANCH}"
echo "VCS_TAG=${VCS_TAG}"
echo "VCS_EXTRA=${VCS_EXTRA}"
echo "VCS_FULL_HASH=${VCS_FULL_HASH}"
echo "VCS_SHORT_HASH=${VCS_SHORT_HASH}"
echo "VCS_WC_MODIFIED=${VCS_WC_MODIFIED}"
echo "VCS_COMMIT_COUNT=${VCS_COMMIT_COUNT}"
echo "VCS_MOST_RECENT_TAGGED_VERSION=${VCS_MOST_RECENT_TAGGED_VERSION}"
echo "VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION=${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}"
echo "VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH=${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}"
echo "VCS_BRANCH_COMMIT_COUNT=${VCS_BRANCH_COMMIT_COUNT}"
echo "VCS_MOST_RECENT_COMMIT_DATE=${VCS_MOST_RECENT_COMMIT_DATE}"
echo "VCS_MOST_RECENT_COMMIT_YEAR=${VCS_MOST_RECENT_COMMIT_YEAR}"
echo ""

# Fetch external libraries
if ! execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources"; then
	execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources" -PBXBuildsContinueAfterErrors=NO
	result=${?}
	if [ $result -ne 0 ]; then
		echo "ERROR: 2nd attempt to fetch external libraries failed with: ${result}"
		exit ${result}
	fi
fi

exit 0
