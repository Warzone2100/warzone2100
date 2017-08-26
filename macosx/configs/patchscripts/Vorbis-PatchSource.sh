#!/bin/sh

if [ -z "$SRCROOT" ]; then
    # Not running from Xcode
    echo "Script is meant to be run by Xcode as part of the build process; SRCROOT is undefined; Exiting"
    exit 1
fi

cd ${SRCROOT}/external/libvorbis

if [ -f "vorbis.diff" ]; then
	echo "info: Already patched"
	exit 0
fi

cp ${SRCROOT}/configs/patchscripts/patches/vorbis.wzpatch vorbis.diff

if ! cat "vorbis.diff" | patch --posix -sNfp0; then
	echo "error: Unable to apply vorbis.diff" >&2
	exit 1
fi

echo "info: Applied vorbis.diff"
exit 0
