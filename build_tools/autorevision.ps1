#Requires -Version 3

Function extractVersionNumberFromGitTag([string]$gitTag)
{
  
	$version_tag = $gitTag
	$version_tag = $version_tag -replace '^v/',''
	$version_tag = $version_tag -replace '^v',''

	$version_tag = $version_tag -replace '_beta.*$',''
	$version_tag = $version_tag -replace '_rc.*$',''

	If ($version_tag -match '^\d+(.\d+){0,2}')
	{
		# value should be in $Matches[0]
		return $Matches[0]
	}
	return $null
}


$VCS_FULL_HASH = (git rev-parse HEAD)
$VCS_SHORT_HASH = ${VCS_FULL_HASH}.Substring(0, 6)
$VCS_TYPE = "git"
$VCS_BASENAME = Get-Item ${PWD}\.. | % { $_.BaseName }
$VCS_BRANCH = (git symbolic-ref --short -q HEAD)
$VCS_TAG = (git describe --exact-match 2> $null)
$VCS_EXTRA = (git describe 2> $null)
$status = (git status --untracked-files=no --porcelain);
$VCS_WC_MODIFIED=($status.length -eq 0)

# get the # of commits in the current history
# IMPORTANT: This value is incorrect if operating from a shallow clone
$VCS_COMMIT_COUNT = (git rev-list --count HEAD 2> $null)

# find the most recent "version" tag in the current history
$tag_skip = 0
$VCS_MOST_RECENT_TAGGED_VERSION = ""
DO
{
	$revision = (git rev-list --tags --skip=${tag_skip} --max-count=1 2> $null)
	$exstatus = $LastExitCode
	If ($exstatus -ne 0  -OR  [string]::IsNullOrEmpty($revision)) {
		break
	}
	$current_tag = (git describe --abbrev=0 --tags "${revision}" 2> $null)
	$exstatus = $LastExitCode
	If ($exstatus -ne 0  -OR  [string]::IsNullOrEmpty($current_tag)) {
		break
	}
    $extracted_version = extractVersionNumberFromGitTag(${current_tag})
	If (-NOT [string]::IsNullOrEmpty(${extracted_version}))
	{
		$VCS_MOST_RECENT_TAGGED_VERSION = ${current_tag}
		break
	}
	$tag_skip++
} While ($tag_skip -le 50000)

$VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION = ""
If (-NOT [string]::IsNullOrEmpty(${VCS_MOST_RECENT_TAGGED_VERSION}))
{
	$VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION = (git rev-list --count `"${VCS_MOST_RECENT_TAGGED_VERSION}`".. 2> $null)
}

# get the commit count on this branch *since* the branch from master
$VCS_BRANCH_COMMIT_COUNT = (git rev-list --count master.. 2> $null)

# get the commit count on master until the branch
$VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH = ""
$first_new_commit_on_branch_since_master = (git rev-list master..) | Select-Object -Last 1
$exstatus = $LastExitCode
If ($exstatus -eq 0)
{
	If ([string]::IsNullOrEmpty($first_new_commit_on_branch_since_master)  -AND  ${VCS_BRANCH_COMMIT_COUNT} -eq 0)
	{
		# The call succeeded, but git returned nothing
		# The number of commits since master is 0, so set VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH
		# to be equal to VCS_COMMIT_COUNT
		$VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH = ${VCS_COMMIT_COUNT}
	}
	Else
	{
		$VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH = (git rev-list --count ${first_new_commit_on_branch_since_master}^ 2> $null)
	}
}

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

#define VCS_COMMIT_COUNT	${VCS_COMMIT_COUNT}
#define VCS_MOST_RECENT_TAGGED_VERSION	"${VCS_MOST_RECENT_TAGGED_VERSION}"
#define VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION	${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}
#define VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH	${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}
#define VCS_BRANCH_COMMIT_COUNT	${VCS_BRANCH_COMMIT_COUNT}

#endif
"@

$content | Set-Content $args[0]

