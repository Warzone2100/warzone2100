#!/bin/bash

cd "${SRCROOT}/../data"

find base mp -name \*.js > "${CONFIGURATION_BUILD_DIR}/jslist.txt"

cd "${CONFIGURATION_BUILD_DIR}"
touch "jslist.txt"
./qtscripttest
echo "warning: check output for test failures" >&2

exit ${?}
