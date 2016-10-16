Write-Host "hey"


$VCS_FULL_HASH = (git rev-parse HEAD)
$VCS_SHORT_HASH = ${VCS_FULL_HASH}.Substring(0, 6)
$VCS_TYPE = "git"
$VCS_BASENAME = Get-Item ${PWD}\.. | % { $_.BaseName }
$VCS_BRANCH = (git symbolic-ref --short -q HEAD)
#$VCS_TAG = (git describe --exact-match 2> /dev/null)
$VCS_EXTRA = (git describe)
$status = (git status --untracked-files=no --porcelain);
$VCS_WC_MODIFIED=($status.length -eq 0)

$content = @"
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
"@

$content | Set-Content $args[0]

