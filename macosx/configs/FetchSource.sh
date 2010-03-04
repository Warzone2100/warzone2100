#!/bin/bash

# Config
DirectorY="$1"
OutDir="$2"
FileName="$3"
SourceDLP="$4"
MD5Sum="$5"


# Make sure we are in the right place
cd "${SRCROOT}"
if [ ! -d "external" ]; then
    mkdir external
fi
cd external

# Do not do more work then we have to
if [ -d "${OutDir}" ]; then
  echo "${OutDir} already exists, skipping"
  exit 0
fi

# Clean if dirty
if [ -d "${DirectorY}" ]; then
  echo "${DirectorY} exists, probably from an earlier failed run" >&2
  #rm -frv "${DirectorY}"
  exit 1
fi

# Fetch
if [ ! -r "${FileName}" ]; then
  echo "Fetching ${SourceDLP}"
  if ! curl -L --connect-timeout "30" -o "${FileName}" "${SourceDLP}"; then
    echo "Unable to fetch ${SourceDLP}" >&2
    exit 1
  fi
fi

# Check our sums
MD5SumLoc=`md5 -q "${FileName}"`
if [ -z "${MD5SumLoc}" ]; then
  echo "Unable to compute md5 for ${FileName}" >&2
  exit 1
elif [ "${MD5SumLoc}" != "${MD5Sum}" ]; then
  echo "MD5 does not match for ${FileName}" >&2
  exit 1
fi

# Unpack
ExtensioN=`echo ${FileName} | sed -e 's:^.*\.\([^.]*\):\1:'`
if [ "${ExtensioN}" = "gz" ]; then
  if ! tar -zxf "${FileName}"; then
    echo "Unpacking ${FileName} failed" >&2
    exit 1
  fi
elif [ "${ExtensioN}" = "bz2" ]; then
  if ! tar -jxf "${FileName}"; then
    echo "Unpacking ${FileName} failed" >&2
    exit 1
  fi
else
  echo "Unable to unpack ${FileName}" >&2
  exit 1
fi

# Reorganize
if [ ! -d "${DirectorY}" ]; then
  echo "Can't find ${DirectorY} to rename" >&2
  exit 1
fi
mv "${DirectorY}" "${OutDir}"

exit 0