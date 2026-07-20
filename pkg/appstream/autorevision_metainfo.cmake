cmake_minimum_required(VERSION 3.16...3.31)

# Automatically update the Release and other info in the net.wz2100.warzone2100.metainfo.xml.in file
#
# Required input defines:
# - CACHEFILE: the path to the autorevision.cache file generated for the build
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
# - TEMPLATE_FILE: the full filename + path for the input net.wz2100.warzone2100.metainfo.xml.in template file
# - OUTPUT_FILE: the full filename + path for the output net.wz2100.warzone2100.metainfo.xml file
#
# And also, passed from the main CMake build:
# - WZ_OUTPUT_NAME_SUFFIX
# - WZ_NAME_SUFFIX
# - WZ_APPSTREAM_ID
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

if(NOT DEFINED WZ_OUTPUT_NAME_SUFFIX)
	message( FATAL_ERROR "Missing expected input define: WZ_OUTPUT_NAME_SUFFIX" )
endif()
if(NOT DEFINED WZ_NAME_SUFFIX)
	message( FATAL_ERROR "Missing expected input define: WZ_NAME_SUFFIX" )
endif()
if(NOT DEFINED WZ_APPSTREAM_ID)
	message( FATAL_ERROR "Missing expected input define: WZ_APPSTREAM_ID" )
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

unset(WZ_METAINFO_RELEASE_VERSION)
unset(WZ_METAINFO_RELEASE_TYPE)
unset(WZ_METAINFO_RELEASE_DATE)

##################################
# Determine the metainfo details

if(DEFINED VCS_TAG AND NOT "${VCS_TAG}" STREQUAL "")
	# We're on an exact tag / tagged release
	set(WZ_METAINFO_RELEASE_VERSION "${VCS_TAG}")
	set(WZ_METAINFO_RELEASE_TYPE "stable")
else()
	set(WZ_METAINFO_RELEASE_VERSION "${VCS_EXTRA}")
	set(WZ_METAINFO_RELEASE_TYPE "development")
endif()
set(WZ_METAINFO_RELEASE_DATE "${VCS_MOST_RECENT_COMMIT_DATE}")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_OUTPUT_NAME_SUFFIX: ${WZ_OUTPUT_NAME_SUFFIX}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_NAME_SUFFIX: ${WZ_NAME_SUFFIX}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_APPSTREAM_ID: ${WZ_APPSTREAM_ID}")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_METAINFO_RELEASE_VERSION: ${WZ_METAINFO_RELEASE_VERSION}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_METAINFO_RELEASE_TYPE: ${WZ_METAINFO_RELEASE_TYPE}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_METAINFO_RELEASE_DATE: ${WZ_METAINFO_RELEASE_DATE}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

