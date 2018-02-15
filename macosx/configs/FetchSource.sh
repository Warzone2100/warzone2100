#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
SourceDLP="$4"
SHA256Sum="$5"
BackupDLP="https://github.com/past-due/wz2100-mac-build-tools/raw/master/sources/"

if ! type -aP shasum > /dev/null; then
	echo "error: Missing command `shasum`. Are you sure Xcode is properly installed?" >&2
	exit 1
fi

# Make sure we are in the right place
cd "${SRCROOT}"
if [ ! -d "external" ]; then
	mkdir external
fi
cd external

# Checks
if [ "${ACTION}" = "clean" ]; then
	# Force cleaning when directed
	rm -fRv "${DirectorY}" "${OutDir}"
	SHA256SumLoc="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
	if [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		rm -fRv "${FileName}"
	fi
	exit 0
elif [ -d "${DirectorY}" ]; then
	# Clean if dirty
	echo "error: ${DirectorY} exists, probably from an earlier failed run" >&2
# 	rm -fRv "${DirectorY}"
	exit 1
elif [[ -d "${OutDir}" ]] && [[ ! -f "${FileName}" ]]; then
	# Clean up when updating versions
	echo "warning: Cached file is outdated or incomplete, removing" >&2
	rm -fR "${DirectorY}" "${OutDir}"
elif [[ -d "${OutDir}" ]] && [[ -f "${FileName}" ]]; then
	# Check to make sure we have the right file
	SHA256SumLoc="$(cat "${OutDir}/.SHA256SumLoc" 2>/dev/null || echo "")"
	if [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		echo "warning: Cached file is outdated or incorrect, removing" >&2
		rm -fR "${DirectorY}" "${OutDir}"
		SHA256SumFle="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
		if [ "${SHA256SumFle}" != "${SHA256Sum}" ]; then
			rm -fR "${FileName}"
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
if [ ! -r "${FileName}" ]; then
	fetchAndCheckSHA256 "${SourceDLP}" "${FileName}" "${SHA256Sum}"
	result=${?}
	if [ $result -ne 0 ]; then
		echo "warning: Fetching from backup source ..." >&2
		fetchAndCheckSHA256 "${BackupDLP}${FileName}" "${FileName}" "${SHA256Sum}"
		result=${?}
		if [ $result -ne 0 ]; then
			echo "error: Unable to fetch ${SourceDLP}" >&2
			exit 1
		fi
	fi
else
	echo "info: ${FileName} already exists, skipping fetch" >&2

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
ExtensioN="$(echo "${FileName}" | sed -e 's:^.*\.\([^.]*\):\1:')"
if [[ "${ExtensioN}" = "gz" ]] || [[ "${ExtensioN}" = "tgz" ]]; then
	if ! tar -zxf "${FileName}"; then
		echo "error: Unpacking ${FileName} failed" >&2
		exit 1
	fi
elif [ "${ExtensioN}" = "bz2" ]; then
	if ! tar -jxf "${FileName}"; then
		echo "error: Unpacking ${FileName} failed" >&2
		exit 1
	fi
else
	echo "error: Unable to unpack ${FileName}" >&2
	exit 1
fi

# Save the sum
echo "${SHA256Sum}" > "${DirectorY}/.SHA256SumLoc"

# Move
if [ ! -d "${DirectorY}" ]; then
	echo "error: Can't find ${DirectorY} to rename" >&2
	exit 1
else
	mv "${DirectorY}" "${OutDir}"
fi

exit 0
