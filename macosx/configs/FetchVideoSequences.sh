#!/bin/bash

# Config
Quality="$1"
OutDir="$2"

bsurl="https://downloads.sf.net/project/warzone2100"

sequencehighnme="sequences.wz"
sequencehighurl="${bsurl}/warzone2100/Videos/high-quality-en/sequences.wz"
sequencehighsha256="90ff552ca4a70e2537e027e22c5098ea4ed1bc11bb7fc94138c6c941a73d29fa"

sequencestdnme="sequences.wz"
sequencestdurl="${bsurl}/warzone2100/Videos/standard-quality-en/sequences.wz"
sequencestdsha256="142ae905be288cca33357a49f42b884c190e828fc0b1b1773ded5dff774f41a3"


. "../src/autorevision.cache"
bldtg="$(echo "${VCS_TAG}" | sed -e 's:/:_:g' -e 's:^v::')_[${VCS_SHORT_HASH}]"


# Check file's sha256 hash
function cksha256 {
	local FileName="${1}"
	local SHA256Sum="${2}"
	local SHA256SumLoc="$(shasum -a 256 "${FileName}" | awk '{ print $1 }')"
	if [ -z "${SHA256SumLoc}" ]; then
		echo "error: Unable to compute sha256 for ${FileName}" >&2
		exit 1
	elif [ "${SHA256SumLoc}" != "${SHA256Sum}" ]; then
		echo "error: SHA256 does not match for ${FileName}" >&2
		exit 1
	fi
}

# Get the appropriate quality sequence

if [[ "${Quality}" == standard ]]; then
	# Standard-quality sequence
	if [[ ! -f "${OutDir}/${sequencestdnme}" ]]; then
		echo "Fetching ${OutDir}/${sequencestdnme}"
		if ! curl -L --connect-timeout "30" -o "${OutDir}/${sequencestdnme}" "$sequencestdurl"; then
			echo "error: Unable to fetch ${sequencestdurl}" >&2
			exit 1
		fi
	else
		echo "${OutDir}/${sequencestdnme} already exists; skipping fetch"
	fi
	cksha256 "${OutDir}/${sequencestdnme}" "${sequencestdsha256}"
	echo "${OutDir}/${sequencestdnme} checksum matches"
elif [[ "${Quality}" == high ]]; then
	# High-quality sequence
	if [[ ! -f "${OutDir}/${sequencehighnme}" ]]; then
		echo "Fetching ${OutDir}/${sequencehighnme}"
		if ! curl -L --connect-timeout "30" -o "${OutDir}/${sequencehighnme}" "${sequencehighurl}"; then
			echo "error: Unable to fetch ${sequencehighurl}" >&2
			exit 1
		fi
	else
		echo "${OutDir}/${sequencehighnme} already exists; skipping fetch"
	fi
	cksha256 "${OutDir}/${sequencehighnme}" "${sequencehighsha256}"
	echo "${OutDir}/${sequencehighnme} checksum matches"
else
	echo "Invalid input parameter for Quality. Acceptable options are: \"standard\", \"high\"."
	exit 1
fi

exit 0
