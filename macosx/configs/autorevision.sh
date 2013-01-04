#!/bin/sh

# This script drives autorevision so that you end up with two
# `autorevision.h` files; one for including in the program and one for
# populating values in the `info.plist` file (some of which are user
# visable).
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


if [[ ! "${VCS_TICK}" = "0" ]]; then
	# If we are not exactly on a tag make the branch look better and use the value for the tag too.
	N_VCS_BRANCH="$(echo "${VCS_BRANCH}" | sed -e 's:remotes/:remote/:' -e 's:master:Master:')"
	sed -e "s:${VCS_BRANCH}:${N_VCS_BRANCH}:" -e "s:${VCS_TAG}:${N_VCS_BRANCH}:" "${cauto}" > "${tauto}"
else
	# When exactly on a tag make the value suitable for users.
	N_VCS_TAG="$(echo "${VCS_TAG}" | sed -e 's:^v::' -e 's:^v/::' | sed -e 's:_beta: Beta :' -e 's:_rc: RC :')"
	sed -e "s:${VCS_TAG}:${N_VCS_TAG}:" "${cauto}" > "${tauto}"
fi


# Output for src/autorevision.h.
./build_tools/autorevision -f -o "${cauto}" -t h > "${xauto}"
if [[ ! -f "${fauto}" ]] || ! cmp -s "${xauto}" "${fauto}"; then
	# Only copy `src/autorevision.h` in if there have been changes.
	cp -a "${xauto}" "${fauto}"
fi

# Output for info.plist prepossessing.
./build_tools/autorevision -f -o "${tauto}" -t xcode > "${xauto}"

exit ${?}
