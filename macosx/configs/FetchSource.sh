#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
SourceDLP="$4"
SHA256Sum="$5"
BackupDLP="http://wz2100.net/~dak180/BuildTools/external/"

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

# Fetch
if [ ! -r "${FileName}" ]; then
	echo "Fetching ${SourceDLP}"
	if ! curl -Lfo "${FileName}" --connect-timeout "30" "${SourceDLP}"; then
		if ! curl -LfOC - --connect-timeout "30" "${BackupDLP}${FileName}"; then
			echo "error: Unable to fetch ${SourceDLP}" >&2
			exit 1
		fi
	fi
else
	echo "${FileName} already exists, skipping" >&2
fi

# Check our sums
SHA256SumLoc="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
if [ -z "${SHA256SumLoc}" ]; then
	echo "error: Unable to compute SHA256 for ${FileName}" >&2
	exit 1
elif [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
	echo "error: SHA256 does not match for ${FileName}" >&2
	exit 1
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
echo "${SHA256SumLoc}" > "${DirectorY}/.SHA256SumLoc"

# Move
if [ ! -d "${DirectorY}" ]; then
	echo "error: Can't find ${DirectorY} to rename" >&2
	exit 1
else
	mv "${DirectorY}" "${OutDir}"
fi

exit 0
