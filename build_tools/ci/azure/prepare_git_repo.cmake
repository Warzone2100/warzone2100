cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

# [preparegitrepo.cmake]
#
# Copyright © 2018-2020 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Azure DevOps: Repo prep
macro(create_all_branches)

	# Keep track of where Azure's checkout task put us.
	# We are on a detached head, and we need to be able to go back to it.
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
					 OUTPUT_VARIABLE build_head
					 OUTPUT_STRIP_TRAILING_WHITESPACE )

	# checkout master to ensure that a local master branch exists
	# "git checkout -qf master"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf master)

	# finally, go back to where we were at the beginning
	# "git checkout -qf ${build_head}"
	execute_process(COMMAND ${GIT_EXECUTABLE} checkout -qf ${build_head})

endmacro()

find_package(Git REQUIRED)

if(DEFINED ENV{TF_BUILD} AND "$ENV{TF_BUILD}" STREQUAL "True")
	create_all_branches()
	message( STATUS "Prepared Azure DevOps Git repo for autorevision" )
else()
	message( WARNING "Azure DevOps environment not detected" )
endif()
