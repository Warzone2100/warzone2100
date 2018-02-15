#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
#
# This script fetches the video sequences, and places them in the data/ directory so they
# are included in the app bundle produced by subsequent builds.
#

##############################
# General Setup


##############################
#

cd macosx

# Fetch the video sequences
# For now, specify the standard quality sequence only
# And download it directly into the ../data/ folder, so it gets picked up by the Xcode app build
configs/FetchVideoSequences.sh standard ../data
result=${?}
if [ $result -ne 0 ]; then
	exit ${?}
fi

exit 0
