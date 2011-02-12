#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This just tries to build warzone.  It does it twice because there are race conditions that may otherwise be triggered.

# Config

cd macosx

if ! xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Warzone" -configuration "StaticAnalyzer"; then
    if ! xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Warzone" -configuration "StaticAnalyzer" -PBXBuildsContinueAfterErrors=NO; then
	    exit ${?}
	fi
fi

exit 0
