#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
#
# This script builds `Warzone.app` and archives it into `Warzone.zip`.
#

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
# Build Warzone

cd macosx

execute_xcodebuild_command  \
 -project Warzone.xcodeproj \
 -target "Warzone" \
 -configuration "Release" \
 -destination "platform=macOS" \
 -PBXBuildsContinueAfterErrors=NO
result=${?}
if [ $result -ne 0 ]; then
	exit ${result}
fi

# Create Warzone.zip
cd build/Release

if [ ! -d "Warzone.app" ]; then
	echo "ERROR: Warzone.app is not present in build/Release"
	exit 1
fi

# For debugging purposes, verify & output some information about the generated Warzone.app
echo "Generated Warzone.app"
generated_infoplist_location="Warzone.app/Contents/Info.plist"
generated_versionnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "${generated_infoplist_location}")
echo "  -> Version Number (CFBundleShortVersionString): ${generated_versionnumber}"
generated_buildnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "${generated_infoplist_location}")
echo "  -> Build Number (CFBundleVersion): ${generated_buildnumber}"
codesign_verify_result=$(codesign --verify --deep --strict --verbose=2 Warzone.app 2>&1)
echo "  -> codesign --verify --deep --strict --verbose=2 Warzone.app"
if [ -n "${codesign_verify_result}" ]; then
	while read -r line; do
		echo "     $line"
	done <<< "$codesign_verify_result"
else
	echo "     (No output?)"
fi


if [ -n "$TRAVIS" ]; then
	# On Travis-CI, emit fold indicators
	echo "travis_fold:start:zip.warzone"
fi
echo "Creating Warzone.zip..."
if ! zip -r Warzone.zip -qdgds 10m Warzone.app; then
	exit ${?}
fi
echo "Warzone.zip created."
if [ -n "$TRAVIS" ]; then
	# On Travis-CI, emit fold indicators
	echo "travis_fold:end:zip.warzone"
fi
cd ../..

cd ..

exit ${?}
