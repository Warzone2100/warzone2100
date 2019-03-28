#!/bin/sh

get_script_dir () {
	# See: https://stackoverflow.com/a/246128
	SOURCE="${BASH_SOURCE[0]}"
	while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
	  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null && pwd )"
	  SOURCE="$(readlink "$SOURCE")"
	  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
	done
	DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null && pwd )"
	echo "$DIR"
}
SCRIPT_DIR="$(get_script_dir)"


VerLib="5.9.7"
OutDir="QT"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
BuiltDLP="https://github.com/past-due/wz2100-mac-build-tools/raw/master/${FileName}"
SHA256Sum="46c385d424512f1d01cd1a7458b4001737a81542ac63c528a171e342aa1c323b"

"${SCRIPT_DIR}/../FetchPrebuilt.sh" "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${SHA256Sum}"
# tar -czf QT-5.9.7.tgz --exclude '.DS_Store' QT-5.9.7

fetchResult=${?}
if [ $fetchResult -ne 0 ]; then
	# Fetch failed
	exit ${fetchResult}
fi

# Clean up Qt frameworks
. "${SCRIPT_DIR}/../patchscripts/QT-FixFrameworkVersions.sh"
