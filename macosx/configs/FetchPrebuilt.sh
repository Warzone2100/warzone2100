#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
BuiltDLP="$4"
SHA256Sum="$5"
Format="$6"
BackupDLP="http://wz2100.net/~dak180/BuildTools/Mac/"

if ! type -aP shasum > /dev/null; then
	echo "error: Missing command `shasum`. Are you sure Xcode is properly installed?" >&2
	exit 0
fi

# Make sure we are in the right place
cd "${SRCROOT}"
if [ ! -d "external" ]; then
	mkdir external
fi
if [ ! -d "prebuilt" ]; then
	mkdir prebuilt
fi

# Checks
if [ "${ACTION}" = "clean" ]; then
	# Force cleaning when directed
	rm -fRv "prebuilt/${DirectorY}" "external/${OutDir}"
	SHA256SumLoc="$(shasum -a 256 "prebuilt/${FileName}" | awk '{ print $1 }')"
	if [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		rm -fRv "prebuilt/${FileName}"
	fi
	exit 0
elif [ -d "prebuilt/${DirectorY}" ]; then
	# Clean if dirty
	echo "error: ${DirectorY} exists, probably from an earlier failed run" >&2
# 	rm -frv "prebuilt/${DirectorY}"
	exit 1
elif [[ -d "external/${OutDir}" ]] && [[ ! -f "prebuilt/${FileName}" ]]; then
	# Clean up when updating versions
	echo "warning: Cached file is outdated or incomplete, removing" >&2
	rm -fR "prebuilt/${DirectorY}" "external/${OutDir}"
elif [[ -d "external/${OutDir}" ]] && [[ -f "prebuilt/${FileName}" ]]; then
	# Check to make sure we have the right file
	SHA256SumLoc="$(cat "external/${OutDir}/.SHA256SumLoc" 2>/dev/null || echo "")"
	if [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		echo "warning: Cached file is outdated or incorrect, removing" >&2
		rm -fR "prebuilt/${DirectorY}" "external/${OutDir}"
		SHA256SumFle=`shasum -a 256 "prebuilt/${FileName}" | awk '{ print $1 }'`
		if [ "${SHA256SumFle}" != "${SHA256Sum}" ]; then
			rm -fR "prebuilt/${FileName}"
		fi
	else
		# Do not do more work then we have to
		echo "${OutDir} already exists, skipping" >&2
		exit 0
	fi
fi

# Fetch
cd prebuilt
if [ ! -f "${FileName}" ]; then
	echo "Fetching ${FileName}"
	if ! curl -LfO --connect-timeout "30" "${BuiltDLP}"; then
		if ! curl -LfOC - --connect-timeout "30" "${BackupDLP}${FileName}"; then
			echo "error: Unable to fetch ${SourceDLP}" >&2
			exit 1
		fi
	fi
else
	echo "${FileName} already exists, skipping" >&2
fi

# SHA256 check
SHA256SumLoc="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
if [ -z "${SHA256SumLoc}" ]; then
	echo "error: Unable to compute SHA256 for ${FileName}" >&2
	exit 1
elif [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
	echo "error: SHA256 does not match for ${FileName}" >&2
	exit 1
fi

# Unpack
if [ "${Format}" = "-dmg" ]; then
    # DMGs must be mounted and then the contents copied

    if [[ ! "0" == "$(type -aP hdiutil &> /dev/null; echo ${?})" ]]; then
        echo "error: hdiutil apparently missing? Unable to unpack DMG: ${FileName}"
        exit 1
    fi
    if [[ ! "0" == "$(type -aP rsync &> /dev/null; echo ${?})" ]]; then
        echo "error: rsync apparently missing? Unable to unpack DMG: ${FileName}"
        exit 1
    fi

    if [ ! -d ".mountedDMGs" ]; then
        mkdir .mountedDMGs
    fi

    # mount the DMG to a local path (under the prebuilt directory)
    MountPoint=".mountedDMGs/${FileName}"

    echo "info: Mounting DMG: \"${FileName}\" -> \"$MountPoint\""
    if hdiutil attach -mountpoint "$MountPoint" "${FileName}" &> /dev/null; then

        # copy the contents of the DMG to the expected "extraction" directory
        # exclude anything beginning with "." in the root directory to avoid errors (ex. ".Trash")
        rsync -av --exclude='.*' "$MountPoint/" "${DirectorY}"

        # unmount the DMG
        hdiutil detach "$MountPoint"
    else
        # hdiutil failed
        echo "error: hdiutil failed to mount the DMG: ${FileName}"
        exit 1
    fi
else
    # default is to treat as tar.gz
    if ! tar -xzf "${FileName}"; then
        echo "error: Unpacking $FileName failed" >&2
        exit 1
    fi
fi

# Save the sum
echo "${SHA256SumLoc}" > "${DirectorY}/.SHA256SumLoc"

# Move
if [ ! -d "${DirectorY}" ]; then
	echo "error: Can't find $DirectorY to rename" >&2
	exit 1
else
	cd ..
	mv "prebuilt/${DirectorY}" "external/${OutDir}"
	touch external/${OutDir}/*
fi

exit 0
