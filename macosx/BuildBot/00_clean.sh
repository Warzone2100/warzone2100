#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script cleans up warzone.

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
# Clean Warzone

cd macosx

execute_xcodebuild_command clean -project Warzone.xcodeproj -target "Warzone" -configuration "Release" -destination "platform=macOS"
result=${?}
if [ $result -ne 0 ]; then
	exit ${result}
fi

rm -rf build/Release

exit ${?}
