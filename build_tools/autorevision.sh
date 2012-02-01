#!/bin/bash

# autorevision.sh - a shell script to get git / hg revisions etc. into binary builds.
# To use pass a path to the desired output file: some/path/to/autorevision.h.
# Note: the script will run at the root level of the repository that it is in.

# Config
TARGETFILE="${1}"

# For git repos
function gitRepo {
	cd "$(git rev-parse --show-toplevel)"
	
	# Is the working copy clean?
	git diff --quiet HEAD &> /dev/null
	WC_MODIFIED="${?}"
	
	# Enumeration of changesets
	VCS_NUM="$(git rev-list --count HEAD)"
	if [ -z "${VCS_NUM}" ]; then
		echo "warning: Counting the number of revisions may be slower due to an outdated git version less than 1.7.2.3. If something breaks, please update it."
		VCS_NUM="$(git rev-list HEAD | wc -l)"
	fi
	
	# The full revision hash
	VCS_FULL_HASH="$(git rev-parse HEAD)"
	
	# The short hash
	VCS_SHORT_HASH="$(echo ${VCS_FULL_HASH} | cut -b 1-7)"
	
	# Current branch
	VCS_URI=`git rev-parse --symbolic-full-name --verify $(git name-rev --name-only --no-undefined HEAD)`
	
	# Current tag (or uri if there is no tag)
	VCS_TAG="$(git describe --exact-match --tags 2>/dev/null)"
	if [ -z "${VCS_TAG}" ]; then
		VCS_TAG="${VCS_URI}"
	fi
	
	# Date of the curent commit
	VCS_DATE="$(git log -1 --pretty=format:%ci)"
}

# For hg repos
function hgRepo {
	cd "$(hg root)"
	
	# Is the working copy clean?
	hg sum | grep -q 'commit: (clean)'
	WC_MODIFIED="${?}"
	
	# Enumeration of changesets
	VCS_NUM="$(hg id -n)"
	
	# The full revision hash
	VCS_FULL_HASH="$(hg log -r ${VCS_NUM} -l 1 --template '{node}\n')"
	
	# The short hash
	VCS_SHORT_HASH="$(hg id -i)"
	
	# Current bookmark (bookmarks are roughly equivalent to git's branches) or branch if no bookmark
	VCS_URI="$(hg id -B | cut -d ' ' -f 1)"
	# Fall back to the branch if there are no bookmarks
	if [ -z "${VCS_URI}" ]; then
		VCS_URI="$(hg id -b)"
	fi
	
	# Current tag (or uri if there is no tag)
	if [ "$(hg log -r ${VCS_NUM} -l 1 --template '{latesttagdistance}\n')" = "0" ]; then
		VCS_TAG="`hg id -t | sed -e 's:qtip::' -e 's:tip::' -e 's:qbase::' -e 's:qparent::' -e "s:$(hg --color never qtop 2>/dev/null)::" | cut -d ' ' -f 1`"
	else
		VCS_TAG="${VCS_URI}"
	fi
	
	# Date of the curent commit
	VCS_DATE="$(hg log -r ${VCS_NUM} -l 1 --template '{date|isodatesec}\n')"
}


if [[ -d .git ]] && [[ ! -z "$(git rev-parse HEAD 2>/dev/null)" ]]; then
	gitRepo
elif [[ -d .hg ]] && [[ ! -z "$(hg root 2>/dev/null)" ]]; then
	hgRepo
else
	echo "error: No repo detected."
	exit 1
fi


cat > "${TARGETFILE}" << EOF
/* ${VCS_FULL_HASH} */
#ifndef AUTOREVISION_H
#define AUTOREVISION_H

#define VCS_NUM			${VCS_NUM}
#define VCS_DATE		${VCS_DATE}
#define VCS_URI			${VCS_URI}
#define VCS_TAG			${VCS_TAG}

#define VCS_FULL_HASH					${VCS_FULL_HASH}
#define VCS_SHORT_HASH					${VCS_SHORT_HASH}

#define VCS_WC_MODIFIED					${WC_MODIFIED}

#endif

EOF
