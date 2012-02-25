#!/bin/bash

cd "${SRCROOT}/../data"

find base mp -iname \*.pie > "${CONFIGURATION_BUILD_DIR}/modellist.txt"

cd "${CONFIGURATION_BUILD_DIR}"
touch "modellist.txt"
./modeltest
echo "warning: check output for test failures" >&2

exit ${?}
