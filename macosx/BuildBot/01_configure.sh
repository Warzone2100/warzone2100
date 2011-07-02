#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This just tries to download the external libs.  It does it twice because they may not download properly the first time.

# Config
wz_conf="StaticAnalyzer"

cd macosx

if ! xcodeindex -project Warzone.xcodeproj -configuration "${wz_conf}"; then
	if ! xcodeindex -project Warzone.xcodeproj -configuration "${wz_conf}"; then
		exit ${?}
	fi
fi


if ! xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Fetch Third Party Sources" -configuration "${wz_conf}"; then
	if ! xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Fetch Third Party Sources" -configuration "${wz_conf}" -PBXBuildsContinueAfterErrors=NO; then
		exit ${?}
	fi
fi

exit 0
