#!/bin/sh

bison --output-file="${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.tab.cpp" "${INPUT_FILE_PATH}" || exit 1
cp "${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.tab.hpp" "${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.h"
exit ${?}
