cmake_minimum_required(VERSION 3.5)

# Automatically update the VERSION_INFO in the warzone2100.rc and the .manifest file
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input rc.in or manifest.in template file
# - OUTPUT_FILE: the full filename + path for the output .rc or .manifest file
#

if(NOT DEFINED CACHEFILE OR "${CACHEFILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: CACHEFILE" )
endif()
if(NOT DEFINED PROJECT_ROOT OR "${PROJECT_ROOT}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: PROJECT_ROOT" )
endif()
if(NOT DEFINED TEMPLATE_FILE OR "${TEMPLATE_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: TEMPLATE_FILE" )
endif()
if(NOT DEFINED OUTPUT_FILE OR "${OUTPUT_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: OUTPUT_FILE" )
endif()

#################################

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++Get build revision info from: ${CACHEFILE}")

# Attempt to get version information from the current build's autorevision cache. Fail if cache is not present.
execute_process(COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${CACHEFILE} -DSKIPUPDATECACHE="1" -DCACHEFORCE="1" -DVAROUT=1 -P "${PROJECT_ROOT}/build_tools/autorevision.cmake"
	WORKING_DIRECTORY "${PROJECT_ROOT}"
	OUTPUT_VARIABLE autorevision_info
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include("${PROJECT_ROOT}/build_tools/autorevision_helpers.cmake")

# Import the autorevision values into the current scope
cmakeSetAutorevisionValues("${autorevision_info}")

unset(RC_FILEVERSION)
unset(RC_PRODUCTVERSION)
unset(RC_StringFileInfo_FileVersion)
unset(RC_StringFileInfo_ProductVersion)
unset(MANIFEST_assemblyIdentityVersion)
unset(RC_StringFileInfo_LegalCopyright)

##################################
# Determine the ProductVersion

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag
	# See if the tag contains a version number
	extractVersionNumberFromGitTag("${VCS_TAG}")
	if(DID_EXTRACT_VERSION)
		# Able to extract a version number from the tag - use it for the PRODUCTVERSION
		set(RC_PRODUCTVERSION "${EXTRACTED_VERSION_MAJOR},${EXTRACTED_VERSION_MINOR},${EXTRACTED_VERSION_PATCH},0")

		# Use the full tag for the StringFileInfo ProductVersion
		set(RC_StringFileInfo_ProductVersion "${VCS_TAG}")
	endif()
endif()

# Extract version info from the most recent tagged version
extractVersionNumberFromGitTag("${VCS_MOST_RECENT_TAGGED_VERSION}")
if(DID_EXTRACT_VERSION)

else()
	message( WARNING "The VCS_MOST_RECENT_TAGGED_VERSION tag does not seem to include a version #; defaulting to 0.0.0" )
endif()

# Determine the build-number component of the version info
if(NOT DEFINED VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION OR "${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}" STREQUAL "")
	set(product_build_number "0")
else()
	set(product_build_number "${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}")
endif()
if(_build_number GREATER 65534)
	# Each component of the ProductVersion on Windows is a 16-bit integer.
	# If the build number is greater than supported, pin it to 65534 and emit a warning
	# (That's a *lot* of commits since the last tagged version...)
	set(product_build_number 65534)
	message( WARNING "The number of commits since the most recent tagged version exceeds 65534; the build-number component of the PRODUCTVERSION will be pinned to 65534." )
endif()

if(NOT DEFINED RC_PRODUCTVERSION)
	# Set the PRODUCT_VERSION to: <MOST_RECENT_TAGGED_VERSION>.<COMMITS_SINCE_MOST_RECENT_TAGGED_VERSION>
	set(RC_PRODUCTVERSION "${EXTRACTED_VERSION_MAJOR},${EXTRACTED_VERSION_MINOR},${EXTRACTED_VERSION_PATCH},${product_build_number}")
endif()

if(NOT DEFINED RC_StringFileInfo_ProductVersion)
	# Set the StringFileInfo ProductVersion to: <VCS_BRANCH> <VCS_SHORTHASH>
	set(RC_StringFileInfo_ProductVersion "${VCS_BRANCH} ${VCS_SHORT_HASH}")
endif()

##################################
# Determine the FileVersion

# Format: "1." (commits-on-master[-until-branch "." commits-on-branch])
if("${VCS_BRANCH}" STREQUAL "master")
	# "1.{commits-on-master}"
	set(RC_StringFileInfo_FileVersion "1.${VCS_COMMIT_COUNT}")
else()
	# "1.{commits-on-master-until-branch}.{commits-on-branch}"
	set(RC_StringFileInfo_FileVersion "1.${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}.${VCS_BRANCH_COMMIT_COUNT}")
endif()

# The FILEVERSION number is a bit complicated:
# Each of the 4 components is a 16-bit integer, so we have to spread the data across the final 3
# For this, we simply use "1,<VCS_COMMIT_COUNT>", with the COMMIT_COUNT potentially wrapping across all 3 final 16-bit integers in a human-readable way
set(_file_version_minor "0")
set(_file_version_patch "0")
if(VCS_COMMIT_COUNT GREATER 2147483647)
	# Reward yourself - we've hit the future
	# Better change / review the MATH code below, because CMake (at least, 32-bit builds?) will overflow on the MATH operations that follow
	message( WARNING "VCS_COMMIT_COUNT exceeds Int32.max, and the following MATH operations may not work properly. Pinning to Int32.max for now - needs to be fixed." )
	set(_file_version_build "2147483647") # for now, pin to Int32.max
else()
	set(_file_version_build "${VCS_COMMIT_COUNT}")
endif()
if(_file_version_build GREATER 65534)
	MATH(EXPR _file_version_patch "${_file_version_build} / 10000")
	MATH(EXPR _file_version_build "${_file_version_build} % 10000")
	if(_file_version_patch GREATER 65534)
		MATH(EXPR _file_version_minor "${_file_version_patch} / 10000")
		MATH(EXPR _file_version_patch "${_file_version_patch} % 10000")
		if(_file_version_minor GREATER 65534)
			message( WARNING "Ran out of room to cram COMMIT_COUNT into VERSION_INFO using current algorithm. Manually increment MAJOR to 2 and augment this code in autorevision_rc.cmake." )
			set(_file_version_minor 65534)
		endif()
	endif()
endif()
set(RC_FILEVERSION "1,${_file_version_minor},${_file_version_patch},${_file_version_build}")
set(MANIFEST_assemblyIdentityVersion "1.${_file_version_minor}.${_file_version_patch}.${_file_version_build}")

##################################
# Determine the other .rc settings

set(RC_StringFileInfo_LegalCopyright "Copyright (C) 2005-${VCS_MOST_RECENT_COMMIT_YEAR} Warzone 2100 Project, Copyright (C) 1999-2004 Eidos Interactive")

set(RC_StringFileInfo_FileDescription "Warzone 2100")
set(RC_StringFileInfo_OriginalFilename "warzone2100.exe")
set(RC_StringFileInfo_ProductName "Warzone 2100")
set(RC_Icon_Filename "warzone2100.ico")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++FILEVERSION: ${RC_FILEVERSION}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++PRODUCTVERSION: ${RC_PRODUCTVERSION}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++(StringInfo) FileVersion: ${RC_StringFileInfo_FileVersion}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++(StringInfo) ProductVersion: ${RC_StringFileInfo_ProductVersion}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++(StringInfo) LegalCopyright: ${RC_StringFileInfo_LegalCopyright}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

