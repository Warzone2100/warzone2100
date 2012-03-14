#!/bin/sh

flex --outfile="${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.cpp" --header-file="${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.h" "${INPUT_FILE_PATH}" || exit 1
exit ${?}
