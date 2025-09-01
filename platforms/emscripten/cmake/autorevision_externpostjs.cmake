cmake_minimum_required(VERSION 3.16...3.31)

# Automatically update the version / build info in the WZ extern-postjs.js file
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input extern-postjs.js.in template file
# - OUTPUT_FILE: the full filename + path for the output extern-postjs.js file
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
execute_process(COMMAND ${CMAKE_COMMAND} "-DCACHEFILE=${CACHEFILE}" -DSKIPUPDATECACHE=1 -DCACHEFORCE=1 -DVAROUT=1 -P "${PROJECT_ROOT}/build_tools/autorevision.cmake"
	WORKING_DIRECTORY "${PROJECT_ROOT}"
	OUTPUT_VARIABLE autorevision_info
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include("${PROJECT_ROOT}/build_tools/autorevision_helpers.cmake")

# Import the autorevision values into the current scope
cmakeSetAutorevisionValues("${autorevision_info}")

unset(EMSCRIPTEN_WZ_VERSIONSTRING)
unset(EMSCRIPTEN_WZ_GITCOMMIT)

##################################
# Extract a version string
# EMSCRIPTEN_WZ_VERSIONSTRING

unset(DID_EXTRACT_VERSION)

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag
	# Try to extract version info from the tag
	extractVersionNumberFromGitTag("${VCS_TAG}")

	if(DID_EXTRACT_VERSION)
		# Use the full tag for the EMSCRIPTEN_WZ_VERSIONSTRING
		set(EMSCRIPTEN_WZ_VERSIONSTRING "${VCS_TAG}")
	endif()
endif()

if(NOT DEFINED EMSCRIPTEN_WZ_VERSIONSTRING)
	# Set the EMSCRIPTEN_WZ_VERSIONSTRING to: <VCS_BRANCH>@<VCS_SHORTHASH>
	set(EMSCRIPTEN_WZ_VERSIONSTRING "${VCS_BRANCH}@${VCS_SHORT_HASH}")
endif()

##################################
# Other variables

set(EMSCRIPTEN_WZ_GITCOMMIT "${VCS_FULL_HASH}")
set(EMSCRIPTEN_WZ_GITBRANCH "${VCS_BRANCH}")
set(EMSCRIPTEN_WZ_GITTAG "${VCS_TAG}")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++EMSCRIPTEN_WZ_VERSIONSTRING: ${EMSCRIPTEN_WZ_VERSIONSTRING}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++EMSCRIPTEN_WZ_GITCOMMIT: ${EMSCRIPTEN_WZ_GITCOMMIT}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++EMSCRIPTEN_WZ_GITBRANCH: ${EMSCRIPTEN_WZ_GITBRANCH}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++EMSCRIPTEN_WZ_GITTAG: ${EMSCRIPTEN_WZ_GITTAG}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

