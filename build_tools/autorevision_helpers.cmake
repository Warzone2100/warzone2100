cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

# Provides functions for:
#	- importing the autorevision data into the CMake local environment
#	- extracting version information from the autorevision data

# Import the autorevision values into the current scope as CMake variables
function(cmakeSetAutorevisionValues _autorevision_info)
	STRING(REGEX REPLACE "\n" ";" _autorevision_info "${_autorevision_info}")
	foreach(_varoutLine ${_autorevision_info})
		STRING(REGEX MATCH "^-- ([^=]+)=(.*)" _line_Matched "${_varoutLine}")
		if(DEFINED CMAKE_MATCH_1 AND NOT "${CMAKE_MATCH_1}" STREQUAL "")
			if(DEFINED CMAKE_MATCH_2)
				set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" PARENT_SCOPE)
			else()
				set(${CMAKE_MATCH_1} "" PARENT_SCOPE)
			endif()
		endif()
	endforeach()
endfunction()

function(extractVersionNumberFromGitTag _gitTag)
	set(version_tag ${_gitTag})

	# Remove "v/" or "v" prefix (as in "v3.2.2"), if present
	STRING(REGEX REPLACE "^v/" "" version_tag "${version_tag}")
	STRING(REGEX REPLACE "^v" "" version_tag "${version_tag}")

	# Remove _beta* or _rc* suffix, if present
	STRING(REGEX REPLACE "_beta.*$" "" version_tag "${version_tag}")
	STRING(REGEX REPLACE "_rc.*$" "" version_tag "${version_tag}")

	# Extract up to a 3-component version # from the beginning of the tag (i.e. 3.2.2)
	STRING(REGEX MATCH "^([0-9]+)(.[0-9]+)?(.[0-9]+)?" version_tag "${version_tag}")

	set(DID_EXTRACT_VERSION OFF PARENT_SCOPE)
	unset(EXTRACTED_VERSION PARENT_SCOPE)
	unset(EXTRACTED_VERSION_MAJOR PARENT_SCOPE)
	unset(EXTRACTED_VERSION_MINOR PARENT_SCOPE)
	unset(EXTRACTED_VERSION_PATCH PARENT_SCOPE)

	if(version_tag)
		set(DID_EXTRACT_VERSION ON PARENT_SCOPE)
		set(EXTRACTED_VERSION ${version_tag} PARENT_SCOPE)
		if(DEFINED CMAKE_MATCH_1)
			set(EXTRACTED_VERSION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
		endif()
		if(DEFINED CMAKE_MATCH_2)
			string(SUBSTRING "${CMAKE_MATCH_2}" 1 -1 _version_minor) # remove the first "." character
			set(EXTRACTED_VERSION_MINOR "${_version_minor}" PARENT_SCOPE)
		endif()
		if(DEFINED CMAKE_MATCH_3)
			string(SUBSTRING "${CMAKE_MATCH_3}" 1 -1 _version_patch) # remove the first "." character
			set(EXTRACTED_VERSION_PATCH "${_version_patch}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

macro(getVersionString)
	if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
		# Use the Tag
		set(VERSION_STRING "${VCS_TAG}")
	elseif(DEFINED VCS_BRANCH AND NOT "${VCS_BRANCH}" STREQUAL "")
		# Use the branch
		set(VERSION_STRING "${VCS_BRANCH}")
	elseif(DEFINED VCS_EXTRA AND NOT "${VCS_EXTRA}" STREQUAL "")
		# Use VCS_EXTRA
		set(VERSION_STRING "${VCS_EXTRA}")
	else()
		set(VERSION_STRING "unknown")
	endif()
endmacro()
