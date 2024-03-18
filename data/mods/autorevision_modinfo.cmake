cmake_minimum_required(VERSION 3.5...3.27)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Automatically update the version compatibility info in a mod-info.json file
# Replaces any instances of @MODINFO_LATEST_VERSION@ with the latest tagged version number
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input mod-info.json.in template file
# - OUTPUT_FILE: the full filename + path for the output mod-info.json file
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

unset(MODINFO_LATEST_VERSION)

##################################
# Determine the ProductVersion

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag
	# See if the tag contains a version number
	extractVersionNumberFromGitTag("${VCS_TAG}")
	if(DID_EXTRACT_VERSION)
		# Able to extract a version number from the tag - use it for the MODINFO_LATEST_TAGGED_VERSION
		set(MODINFO_LATEST_VERSION "${EXTRACTED_VERSION_MAJOR}.${EXTRACTED_VERSION_MINOR}.${EXTRACTED_VERSION_PATCH}")
	endif()
endif()

# Extract version info from the most recent tagged version
extractVersionNumberFromGitTag("${VCS_MOST_RECENT_TAGGED_VERSION}")
if(DID_EXTRACT_VERSION)

else()
	message( WARNING "The VCS_MOST_RECENT_TAGGED_VERSION tag does not seem to include a version" )
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++Warning: The VCS_MOST_RECENT_TAGGED_VERSION tag does not seem to include a version")
endif()

if(NOT DEFINED MODINFO_LATEST_VERSION)
	# Set the MODINFO_LATEST_VERSION to: <MOST_RECENT_TAGGED_VERSION>
	set(MODINFO_LATEST_VERSION "${EXTRACTED_VERSION_MAJOR}.${EXTRACTED_VERSION_MINOR}.${EXTRACTED_VERSION_PATCH}")
endif()

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++MODINFO_LATEST_VERSION: ${MODINFO_LATEST_VERSION}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

