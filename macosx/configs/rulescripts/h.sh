#!/bin/sh

export PATH=/sw/lib/qt4-mac/bin:/sw/lib/qt4-x11/bin:/opt/local/bin:${PATH}
moc -o "${DERIVED_FILES_DIR}/${INPUT_FILE_BASE}.moc.cpp" "${INPUT_FILE_PATH}" || exit 1
# Files w/ "Q_OBJECT"
