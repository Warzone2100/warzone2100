#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
BuiltDLP="$4"
MD5Sum="$5"
BackupDLP="http://wz2100.net/~dak180/BuildTools/Mac/"


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
    MD5SumLoc="$(md5 -q "prebuilt/${FileName}")"
    if [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
        rm -fRv "prebuilt/${FileName}"
    fi
    exit 0
elif [ -d "prebuilt/${DirectorY}" ]; then
    # Clean if dirty
    echo "error: ${DirectorY} exists, probably from an earlier failed run" >&2
#     rm -frv "prebuilt/${DirectorY}"
    exit 1
elif [[ -d "external/${OutDir}" ]] && [[ ! -f "prebuilt/${FileName}" ]]; then
    # Clean up when updating versions
    echo "warning: Cached file is outdated or incomplete, removing" >&2
    rm -fR "prebuilt/${DirectorY}" "external/${OutDir}"
elif [[ -d "external/${OutDir}" ]] && [[ -f "prebuilt/${FileName}" ]]; then
    # Check to make sure we have the right file
    MD5SumLoc="$(cat "external/${OutDir}/.MD5SumLoc" 2>/dev/null || echo "")"
    if [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
        echo "warning: Cached file is outdated or incorrect, removing" >&2
        rm -fR "prebuilt/${DirectorY}" "external/${OutDir}"
		MD5SumFle=`md5 -q "prebuilt/${FileName}"`
		if [ "${MD5SumFle}" != "${MD5Sum}" ]; then
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

# MD5 check
MD5SumLoc="$(md5 -q "${FileName}")"
if [ -z "${MD5SumLoc}" ]; then
    echo "error: Unable to compute md5 for ${FileName}" >&2
    exit 1
elif [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
    echo "error: MD5 does not match for ${FileName}" >&2
    exit 1
fi

# Unpack
if ! tar -xzf "${FileName}"; then
    echo "error: Unpacking $FileName failed" >&2
    exit 1
fi

# Save the sum
echo "${MD5SumLoc}" > "${DirectorY}/.MD5SumLoc"

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
