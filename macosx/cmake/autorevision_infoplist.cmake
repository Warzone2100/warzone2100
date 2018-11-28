cmake_minimum_required(VERSION 3.5)

# Automatically update the VERSION_INFO in the Warzone Info.plist file
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input Warzone-Info.plist.in template file
# - OUTPUT_FILE: the full filename + path for the output Info.plist file
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

function(VALIDATE_INTEGER var default_value)
	if (NOT ${var} MATCHES "^[0-9]+$")
		message( WARNING "warning: ${var} (\"${${var}}\") is non-integer; defaulting to \"${default_value}\"" )
		set(${var} ${default_value} PARENT_SCOPE)
	endif()
endfunction()

#################################

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++Get build revision info from: ${CACHEFILE}")

# Attempt to get version information from the current build's autorevision cache. Fail if cache is not present.
execute_process(COMMAND ${CMAKE_COMMAND} "-DCACHEFILE=${CACHEFILE}" -DSKIPUPDATECACHE=1 -DCACHEFORCE=1 -DVAROUT=1 -P "${PROJECT_ROOT}/build_tools/autorevision.cmake"
	WORKING_DIRECTORY "${PROJECT_ROOT}"
	OUTPUT_VARIABLE autorevision_info
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include("${PROJECT_ROOT}/build_tools/autorevision_helpers.cmake")

# Import the autorevision values into the current scope
cmakeSetAutorevisionValues("${autorevision_info}")

unset(MAC_VERSION_NUMBER)
unset(MAC_BUILD_NUMBER)
unset(WZINFO_NSHumanReadableCopyright)
unset(WZINFO_CFBundleGetInfoString)

##################################
# Extract a version number from the most recent version-like tag (to be used as the CFBundleShortVersionString)
# MAC_VERSION_NUMBER

unset(DID_EXTRACT_VERSION)

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag
	# Try to extract version info from the tag
	extractVersionNumberFromGitTag("${VCS_TAG}")
endif()

if(NOT DID_EXTRACT_VERSION)
	# Extract version info from the most recent tagged version
	extractVersionNumberFromGitTag("${VCS_MOST_RECENT_TAGGED_VERSION}")
endif()

if(DID_EXTRACT_VERSION)
	if(NOT EXTRACTED_VERSION_MINOR)
		set(EXTRACTED_VERSION_MINOR 0)
	endif()
	if(NOT EXTRACTED_VERSION_PATCH)
		set(EXTRACTED_VERSION_PATCH 0)
	endif()
	set(MAC_VERSION_NUMBER "${EXTRACTED_VERSION_MAJOR}.${EXTRACTED_VERSION_MINOR}.${EXTRACTED_VERSION_PATCH}")
else()
	message( WARNING "warning: The VCS_MOST_RECENT_TAGGED_VERSION tag does not seem to include a version #; defaulting to 0.0.0" )
	set(MAC_VERSION_NUMBER "0.0.0")
endif()

##################################
# Construct a build number (for the CFBundleVersion)
# MAC_BUILD_NUMBER

# Format: "1." (commits-on-master[-until-branch "." commits-on-branch])
if("${VCS_BRANCH}" STREQUAL "master")
	# "1.{commits-on-master}.0"
	VALIDATE_INTEGER(VCS_COMMIT_COUNT 0)
	set(MAC_BUILD_NUMBER "1.${VCS_COMMIT_COUNT}.0")
else()
	# "1.{commits-on-master-until-branch}.{commits-on-branch}"
	VALIDATE_INTEGER(VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH 0)
	VALIDATE_INTEGER(VCS_BRANCH_COMMIT_COUNT 0)
	set(MAC_BUILD_NUMBER "1.${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}.${VCS_BRANCH_COMMIT_COUNT}")
endif()

##################################
# Determine the other Info.plist settings

set(WZINFO_NSHumanReadableCopyright "Copyright © 2005-${VCS_MOST_RECENT_COMMIT_YEAR} Warzone 2100 Project, Copyright © 1999-2004 Eidos Interactive")
set(WZINFO_CFBundleGetInfoString "${VCS_TAG} :${VCS_SHORT_HASH}:, ${WZINFO_NSHumanReadableCopyright}")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++MAC_VERSION_NUMBER: ${MAC_VERSION_NUMBER}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++MAC_BUILD_NUMBER: ${MAC_BUILD_NUMBER}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++NSHumanReadableCopyright: ${WZINFO_NSHumanReadableCopyright}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++CFBundleGetInfoString: ${WZINFO_CFBundleGetInfoString}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

