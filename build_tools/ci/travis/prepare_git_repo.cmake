cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

# [preparegitrepo.cmake]
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Travis-CI: Repo prep
# (Travis limits Git clone depth, but we need the full history *and* the master branch for autorevision)
# See: https://stackoverflow.com/a/44036486
macro(create_all_branches)

	# Keep track of where Travis put us.
	# We are on a detached head, and we need to be able to go back to it.
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
					 OUTPUT_VARIABLE build_head
					 OUTPUT_STRIP_TRAILING_WHITESPACE )

	# Fetch all the remote branches. Travis clones with `--depth`, which
	# implies `--single-branch`, so we need to overwrite remote.origin.fetch to
	# do that.
	# "git config --replace-all remote.origin.fetch +refs/heads/*:refs/remotes/origin/*"
	execute_process( COMMAND ${GIT_EXECUTABLE} config --replace-all remote.origin.fetch +refs/heads/*:refs/remotes/origin/*
					)

	# echo "travis_fold:start:git.fetch.unshallow"
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "travis_fold:start:git.fetch.unshallow")
	# "git fetch --unshallow"
	execute_process(COMMAND ${GIT_EXECUTABLE} fetch --unshallow)
	# echo "travis_fold:end:git.fetch.unshallow"
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "travis_fold:end:git.fetch.unshallow")

	# also fetch the tags
	# echo "travis_fold:start:git.fetch.tags"
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "travis_fold:start:git.fetch.tags")
	# "git fetch --tags"
	execute_process(COMMAND ${GIT_EXECUTABLE} fetch --tags)
	# echo "travis_fold:end:git.fetch.tags"
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "travis_fold:end:git.fetch.tags")

	# checkout master to ensure that a local master branch exists
	# "git checkout -qf master"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf master)

	# finally, go back to where we were at the beginning
	# "git checkout -qf ${build_head}"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf ${build_head})

endmacro()

find_package(Git REQUIRED)

if(DEFINED ENV{CI} AND "$ENV{CI}" STREQUAL "true" AND DEFINED ENV{TRAVIS} AND "$ENV{TRAVIS}" STREQUAL "true")
	create_all_branches()
	message( STATUS "Prepared Travis-CI Git repo for autorevision" )
else()
	message( WARNING "Travis-CI environment not detected" )
endif()
