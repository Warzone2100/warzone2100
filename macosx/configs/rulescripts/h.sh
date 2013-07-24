#!/bin/sh

export PATH="${SRCROOT}/external/QT/usr/bin":${PATH}
moc -o "${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.moc.cpp" "${INPUT_FILE_PATH}" || exit 1
# Files w/ "Q_OBJECT"
