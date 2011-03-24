#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script sets the name of the .dmg and dSYM bundle as it uploads them and does some link magic.

# Config
rtag="$(git branch --no-color | sed -e '/^[^*]/d' -e 's:* \(.*\):\1:')"
uurl="buildbot@buildbot.wz2100.net"
opth="${rtag}/mac/"
rpth="public_html/"
lpth="macosx/build/dmgout/out/"
revt="-$(git rev-parse -q --short --verify HEAD)"
dmg_bn="warzone2100"
dmg_nv="-novideo.dmg"
tar_dS="-dSYM.tar.gz"


# Set bran
if [ -z ${rtag} ]; then
	echo "Must supply the branch name being built."
	exit 1
fi
bran="-${rtag}"


# Upload the dSYM bundle
echo "Starting to upload the dSYM bundle."
if ! scp -pqCl 320 ${lpth}${dmg_bn}${tar_dS} ${uurl}:${rpth}${opth}${dmg_bn}${bran}${revt}${tar_dS}; then
	exstat="${?}"
	echo "error: Upload did not complete!"
	exit ${exstat}
fi

# Upload the .dmg
echo "Starting to upload the dmg image."
if ! scp -pqCl 320 ${lpth}${dmg_bn}${dmg_nv} ${uurl}:${rpth}${opth}${dmg_bn}${bran}${revt}.dmg; then
	exstat="${?}"
	echo "error: Upload did not complete!"
	exit ${exstat}
fi


# Link up the current .dmg and dSYM bundle
if ! ssh ${uurl} -C "cd ${rpth} && ln -fs ${opth}${dmg_bn}${bran}${revt}.dmg ${dmg_bn}${bran}-current.dmg && ln -fs ${opth}${dmg_bn}${bran}${revt}${tar_dS} ${dmg_bn}${bran}-current${tar_dS}"; then
	exstat="${?}"
	echo "error: Failed to link!"
	exit ${exstat}
fi

exit 0
