#!/bin/bash

cd "${SRCROOT}/../data"

find base mp -name game.map > "${CONFIGURATION_BUILD_DIR}/maplist.txt"

cd "${CONFIGURATION_BUILD_DIR}"
touch "maplist.txt"
./maptest
echo "warning: check output for test failures" >&2

exit ${?}
