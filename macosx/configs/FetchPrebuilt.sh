#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
BuiltDLP="$4"
SHA256Sum="$5"
Format="$6"
BackupDLP="https://github.com/past-due/wz2100-mac-build-tools/raw/master/sources/"

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

function checkFileSHA256 {
	local FileName="$1"
	local SHA256Sum="$2"

	local SHA256SumLoc="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
	if [ -z "${SHA256SumLoc}" ]; then
		echo "warning: Unable to compute SHA256 for ${FileName}" >&2
		return 1
	elif [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		echo "warning: SHA256 does not match for ${FileName}; (received: ${SHA256SumLoc}) (expecting: ${SHA256Sum}) (file size: $(stat -f%z "${FileName}"))" >&2
		rm -f "${FileName}"
		return 1
	fi

	return 0
}

function fetchAndCheckSHA256 {
	local DLURL="$1"
	local FileName="$2"
	local SHA256Sum="$3"

	# Fetch the file
	echo "info: Fetching: ${DLURL}" >&2
	curl -Lfo "${FileName}" --connect-timeout "30" "${DLURL}"
	local result=${?}
	if [ $result -ne 0 ]; then
		echo "warning: Fetch failed: ${DLURL}" >&2
		return ${result}
	fi

	# Check SHA256 sum of downloaded file
	checkFileSHA256 "${FileName}" "${SHA256Sum}"
	result=${?}
	if [ $result -ne 0 ]; then
		return ${result}
	fi

	return 0
}

# Fetch and check SHA256 sum
cd prebuilt
if [ ! -f "${FileName}" ]; then
	fetchAndCheckSHA256 "${BuiltDLP}" "${FileName}" "${SHA256Sum}"
	result=${?}
	if [ $result -ne 0 ]; then
		echo "warning: Fetching from backup source ..." >&2
		fetchAndCheckSHA256 "${BackupDLP}${FileName}" "${FileName}" "${SHA256Sum}"
		result=${?}
		if [ $result -ne 0 ]; then
			echo "error: Unable to fetch ${BuiltDLP}" >&2
			exit 1
		fi
	fi
else
	echo "info: ${FileName} already exists, skipping" >&2

	# Check SHA256
	checkFileSHA256 "${FileName}" "${SHA256Sum}"
	result=${?}
	if [ $result -ne 0 ]; then
		# SHA256 of existing file does not match expected
		echo "error: Existing \"${FileName}\" does not match expected SHA256 hash" >&2
		exit ${result}
	fi
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
echo "${SHA256Sum}" > "${DirectorY}/.SHA256SumLoc"

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
