cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

# [preparegitrepo.cmake]
#
# Copyright © 2018-2020 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# GitHub Actions: Repo prep
macro(create_all_branches)

	# Keep track of where Travis put us.
	# We are on a detached head, and we need to be able to go back to it.
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
					 OUTPUT_VARIABLE build_head
					 OUTPUT_STRIP_TRAILING_WHITESPACE )
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "build_head: ${build_head}")

	# "git fetch --prune"
	execute_process(COMMAND ${GIT_EXECUTABLE} fetch --prune --progress)

	# also fetch the tags
	# "git fetch --tags"
	execute_process(COMMAND ${GIT_EXECUTABLE} fetch --tags)

	# checkout master to ensure that a local master branch exists
	# "git checkout -qf master"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf master)

	# finally, go back to where we were at the beginning
	# "git checkout -qf ${build_head}"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf ${build_head})

endmacro()

find_package(Git REQUIRED)

if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
	create_all_branches()
	message( STATUS "Prepared GitHub Actions Git repo for autorevision" )
else()
	message( WARNING "GitHub Actions environment not detected" )
endif()
