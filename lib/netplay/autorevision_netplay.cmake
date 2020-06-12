cmake_minimum_required(VERSION 3.5)

# Automatically update the NETCODE_VERSION in the netplay_config.h file
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input netplay_config.h.in template file
# - OUTPUT_FILE: the full filename + path for the output netplay_config.h file
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

function(VALIDATE_INTEGER var)
	if (NOT DEFINED ${var} OR ${var} STREQUAL "")
		message(FATAL_ERROR "Missing required: ${var}")
	endif()
	if (NOT ${var} MATCHES "^[0-9]+$")
		message(FATAL_ERROR "${var} is not a number?: \"${${var}}\"")
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

unset(NETCODE_VERSION_MAJOR)
unset(NETCODE_VERSION_MINOR)

##################################
# Calculate the NETCODE version
# Rules:
# - tagged (release) builds are versioned as:
#	- NETCODE_VERSION_MAJOR: 0x4000
#	- NETCODE_VERSION_MINOR: VCS_TAG_TAG_COUNT
# - master branch builds are versioned as:
#	- NETCODE_VERSION_MAJOR: 0x10a0
#	- NETCODE_VERSION_MINOR: VCS_COMMIT_COUNT
# - any other builds (other branches, forks, etc)
#	- NETCODE_VERSION_MAJOR: 0x1000
#	- NETCODE_VERSION_MINOR: 1

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag / tagged release
	VALIDATE_INTEGER(VCS_TAG_TAG_COUNT)
	set(NETCODE_VERSION_MAJOR "0x4000")
	set(NETCODE_VERSION_MINOR ${VCS_TAG_TAG_COUNT})
else()
	if("${VCS_BRANCH}" STREQUAL "master")
		# master branch build
		VALIDATE_INTEGER(VCS_COMMIT_COUNT)
		set(NETCODE_VERSION_MAJOR "0x10a0")
		set(NETCODE_VERSION_MINOR ${VCS_COMMIT_COUNT})
	else()
		# any other builds (other branches, forks, etc)
		set(NETCODE_VERSION_MAJOR "0x1000")
		set(NETCODE_VERSION_MINOR 1)
	endif()
endif()

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++NETCODE_VERSION_MAJOR: ${NETCODE_VERSION_MAJOR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++NETCODE_VERSION_MINOR: ${NETCODE_VERSION_MINOR}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

