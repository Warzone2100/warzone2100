$VCS_FULL_HASH = (git rev-parse HEAD)
$VCS_SHORT_HASH = (echo "${VCS_FULL_HASH}" | cut -b 1-7)
$VCS_TYPE = "git"
$VCS_BASENAME = (basename ${PWD}\..)
$VCS_BRANCH = (git symbolic-ref --short -q HEAD)
#$VCS_TAG = (git describe --exact-match 2> /dev/null)
$VCS_EXTRA = (git describe)
test -z "$(git status --untracked-files=no --porcelain)"
$VCS_WC_MODIFIED="${?}"

@"
#ifndef AUTOREVISION_H
#define AUTOREVISION_H
#define VCS_TYPE		"${VCS_TYPE}"
#define VCS_TYPE		"${VCS_TYPE}"
#define VCS_BASENAME	"${VCS_BASENAME}"
#define VCS_BRANCH		"${VCS_BRANCH}"
#define VCS_TAG			"${VCS_TAG}"
#define VCS_EXTRA       "${VCS_EXTRA}"


#define VCS_FULL_HASH		"${VCS_FULL_HASH}"
#define VCS_SHORT_HASH		"${VCS_SHORT_HASH}"

#define VCS_WC_MODIFIED		${VCS_WC_MODIFIED}
#endif
"@ > ..\src\autorevision.h
