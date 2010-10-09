#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script cleans up warzone but not the libs so builds will go faster.

# Config

cd macosx

xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Warzone" -configuration "Release" -nodependencies clean
exit ${?}
