#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script builds Warzone and makes the actual .dmg and dSYM bundle.  It is very important to keep the dSYM bundle with it's .dmg; it contains irreplaceable debug info (and, no another build even made right after will not work).

# Config

cd macosx

xcodebuild -project Warzone.xcodeproj -parallelizeTargets -target "Make DMGs for Release" -configuration "StaticAnalyzer" -PBXBuildsContinueAfterErrors=NO
exit ${?}
