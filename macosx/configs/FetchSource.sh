#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
SourceDLP="$4"
MD5Sum="$5"
BackupDLP="http://wz2100.net/~dak180/BuildTools/external/"


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
    MD5SumLoc="$(md5 -q "${FileName}")"
    if [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
        rm -fRv "${FileName}"
    fi
    exit 0
elif [ -d "${DirectorY}" ]; then
    # Clean if dirty
    echo "error: ${DirectorY} exists, probably from an earlier failed run" >&2
#     rm -fRv "${DirectorY}"
    exit 1
elif [[ -d "${OutDir}" ]] && [[ ! -f "${FileName}" ]]; then
    # Clean up when updating versions
    echo "warning: Cached file is outdated or incomplete, removing" >&2
    rm -fR "${DirectorY}" "${OutDir}"
elif [[ -d "${OutDir}" ]] && [[ -f "${FileName}" ]]; then
    # Check to make sure we have the right file
    MD5SumLoc="$(cat "${OutDir}/.MD5SumLoc" 2>/dev/null || echo "")"
    if [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
        echo "warning: Cached file is outdated or incorrect, removing" >&2
        rm -fR "${DirectorY}" "${OutDir}"
		MD5SumFle="$(md5 -q "${FileName}")"
		if [ "${MD5SumFle}" != "${MD5Sum}" ]; then
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
MD5SumLoc="$(md5 -q "${FileName}")"
if [ -z "${MD5SumLoc}" ]; then
    echo "error: Unable to compute md5 for ${FileName}" >&2
    exit 1
elif [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
    echo "error: MD5 does not match for ${FileName}" >&2
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
echo "${MD5SumLoc}" > "${DirectorY}/.MD5SumLoc"

# Move
if [ ! -d "${DirectorY}" ]; then
    echo "error: Can't find ${DirectorY} to rename" >&2
    exit 1
else
    mv "${DirectorY}" "${OutDir}"
fi

exit 0
