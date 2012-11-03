#!/bin/sh

# Config
export PATH=$PATH:/sw/bin:/opt/local/bin
sauto="${SRCROOT}/../src/autorevision.h"
bauto="${OBJROOT}/autorevision.h"
tauto="${SRCROOT}/../autorevision.tmp"
tautoo="${OBJROOT}/autorevision.tmp"

cd "${SRCROOT}/.."

if ! ./build_tools/autorevision -o "${tauto}" -t h > "${sauto}"; then
	exit ${?}
fi


. "${tauto}"


if [[ ! "${VCS_TICK}" = "0" ]]; then
	N_VCS_BRANCH="$(echo "${VCS_BRANCH}" | sed -e 's:remotes/:remote/:' -e 's:master:Master:')"
	sed -e "s:${VCS_BRANCH}:${N_VCS_BRANCH}:" -e "s:${VCS_TAG}:${N_VCS_BRANCH}:" "${tauto}" > "${tautoo}"
else
	N_VCS_TAG="$(echo "${VCS_TAG}" | sed -e 's:^v::' -e 's:^v/::' | sed -e 's:_beta: Beta :' -e 's:_rc: RC :')"
	sed -e "s:${VCS_TAG}:${N_VCS_TAG}:" "${tauto}" > "${tautoo}"
fi


./build_tools/autorevision -f -o "${tautoo}" -t xcode > "${bauto}"

exit ${?}
