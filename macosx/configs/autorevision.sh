#!/bin/sh

# This script drives autorevision so that you end up with two
# `autorevision.h` files; one for including in the program and one for
# populating values in the `info.plist` file (some of which are user
# visible).
# The cache file is generated in a repo and is read from
# unconditionally when building from a tarball.

# Config
export PATH=${PATH}:/sw/bin:/opt/local/bin:/usr/local/bin:/usr/local/git/bin
sauto="${DERIVED_FILE_DIR}/autorevision.h"
fauto="${SRCROOT}/../src/autorevision.h"
xauto="${OBJROOT}/autorevision.h"
cauto="${SRCROOT}/../src/autorevision.cache"
tauto="${OBJROOT}/autorevision.tmp"


cd "${SRCROOT}/.."

# Output the autorevision cache or display it in a tarball.
if ! ./build_tools/autorevision -o "${cauto}" -t sh; then
	exit ${?}
fi


# Source the cache to allow for value manipulation.
. "${cauto}"


# Process the values for Xcode

extractVersionNumberFromGitTag() {
	local version_tag="$1"

	# Remove "v/" or "v" prefix (as in "v3.2.2"), if present
	version_tag="$(echo "${version_tag}" | sed -e 's:^v/::' -e 's:^v::')"

	# Remove _beta* or _rc* suffix, if present
	version_tag="$(echo "${version_tag}" | sed -e 's:_beta.*$::' -e 's:_rc.*$::')"

	# Extract up to a 3-component version # from the beginning of the tag (i.e. 3.2.2)
	version_tag="$(echo "${version_tag}" | grep -Eo '^\d+(.\d+){0,2}')"

	if [ -n "${version_tag}" ]; then
		echo "${version_tag}"
		return 0
	else
		return 1 # failed to extract version #
	fi
}

# Make the branch look better.
if [ -n "${VCS_BRANCH}" ]; then
	N_VCS_BRANCH="$(echo "${VCS_BRANCH}" | sed -e 's:remotes/:remote/:' -e 's:master:Master:')"
	sed -e "s:\"${VCS_BRANCH}\":\"${N_VCS_BRANCH}\":" "${cauto}" > "${tauto}"
fi

if [ -n "${VCS_TAG}" ]; then
	# When exactly on a tag make the value suitable for users.
	N_VCS_TAG="$(echo "${VCS_TAG}" | sed -e 's:^v/::' -e 's:^v::' | sed -e 's:_beta: Beta :' -e 's:_rc: RC :')"
	sed -e "s:\"${VCS_TAG}\":\"${N_VCS_TAG}\":" "${cauto}" > "${tauto}"
fi

# Extract a version number from the most recent version-like tag (to be used as the CFBundleShortVersionString)
MAC_VERSION_NUMBER="$(extractVersionNumberFromGitTag "${VCS_MOST_RECENT_TAGGED_VERSION}")"
if [ -z "${MAC_VERSION_NUMBER}" ]; then
	# Tag does not start with identifiable version # components
	echo "warning: The VCS_MOST_RECENT_TAGGED_VERSION tag does not seem to include a version #; defaulting to 0.0.0"
	MAC_VERSION_NUMBER="0.0.0"
fi

## To increment the 3rd component of the Version# if there are commits since the most recent tagged version,
## uncomment the code below
#if [ "$VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION" -gt "0" ]; then
#	# Since there have been commits since the most recent tag, increment the MAC_VERSION_NUMBER
#
#	VERSION_A=$(echo "${MAC_VERSION_NUMBER}" | awk -F. '{print $1}')
#	VERSION_B=$(echo "${MAC_VERSION_NUMBER}" | awk -F. '{print $2}')
#	VERSION_C=$(echo "${MAC_VERSION_NUMBER}" | awk -F. '{print $3}')
#
#	if [ -n "${VERSION_C}" ]; then
#		VERSION_C=$(($VERSION_C + 1))
#	elif [ -n "${VERSION_B}" ]; then
#		VERSION_C="1"
#	elif [ -n "${VERSION_A}" ]; then
#		VERSION_C="1"
#		VERSION_B="0"
#	fi
#	MAC_VERSION_NUMBER="${VERSION_A}.${VERSION_B}.${VERSION_C}"
#fi
echo "info: Using Version # \"${MAC_VERSION_NUMBER}\" (most recent version tag: ${VCS_MOST_RECENT_TAGGED_VERSION})"

# Construct a build number (for the CFBundleVersion)
# Format: "1." (commits-on-master[-until-branch "." commits-on-branch])
if [ "${VCS_BRANCH}" = "master" ]; then
	# "1.{commits-on-master}"
	MAC_BUILD_NUMBER="1.${VCS_COMMIT_COUNT}"
else
	# "1.{commits-on-master-until-branch}.{commits-on-branch}"
	MAC_BUILD_NUMBER="1.${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}.${VCS_BRANCH_COMMIT_COUNT}"
fi
echo "info: Using Build # \"${MAC_BUILD_NUMBER}\""


# Output for src/autorevision.h.
./build_tools/autorevision -f -o "${cauto}" -t h > "${xauto}"
if [[ ! -f "${fauto}" ]] || ! cmp -s "${xauto}" "${fauto}"; then
	# Only copy `src/autorevision.h` in if there have been changes.
	cp -a "${xauto}" "${fauto}"
fi

# Output for info.plist prepossessing.
./build_tools/autorevision -f -o "${tauto}" -t xcode > "${xauto}"

# Modify the output header for info.plist preprocessing to insert the MAC_BUILD_NUMBER and MAC_VERSION_NUMBER
infoplist_header_additions="#define MAC_BUILD_NUMBER	${MAC_BUILD_NUMBER}\\
#define MAC_VERSION_NUMBER	${MAC_VERSION_NUMBER}"
perl -i.bak -pe 's/## AUTOINSERT MARK/'"${infoplist_header_additions}"'/g' "${xauto}"

exit ${?}
