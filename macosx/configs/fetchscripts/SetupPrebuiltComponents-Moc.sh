#!/bin/bash

export PATH="${SRCROOT}/external/QT/usr/bin":${PATH}
if [ "${BACKEND}" = "BACKEND_QT" ]  || [ "${BACKEND}" = "BACKEND_SDL" ]; then
    echo "Checking for moc..."
    if ! type -aP moc; then
        echo "error: Fatal inability to properly moc the code. (Insure your path is not doing something strange or risk more mocing.)" >&2
        echo "PATH=${PATH}"
        exit 1
    elif [[ "$(moc -v 2>&1 | sed -e 's:^Qt Meta Object Compiler version \([0-9]*\).*$:\1:')" < "63" ]]; then
        echo "error: Moc is not properly installed" >&2
        exit 1
    fi
moc -v
else
    echo "note: Moc is currently only needed when using the Qt backend."
fi
exit 0
