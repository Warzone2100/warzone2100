#!/bin/bash

# This script is included by the other scripts in this directory.
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

##############################
# General Setup

XCODEBUILD="xcodebuild"

# Determine if xcpretty is present
XCPRETTY_PATH=$(command -v xcpretty 2> /dev/null)

# If xcpretty is present, use it to format xcodebuild output
XCPRETTY=
if [ -n "$XCPRETTY_PATH" ]; then
	XCPRETTY="xcpretty -c"

	# On Travis-CI, use xcpretty-travis-formatter
	if [ -n "$TRAVIS" ]; then
		XCPRETTY="$XCPRETTY -f `xcpretty-travis-formatter`"
	fi
fi


##############################
# Helper Functions

execute_xcodebuild_command () {
	if [ -n "$XCPRETTY" ]; then
		set -x
		set -o pipefail && $XCODEBUILD "$@" | $XCPRETTY
	else
		set -x
		$XCODEBUILD "$@"
	fi
	local RESULT=${?}
	set +x
	return ${RESULT}
}
