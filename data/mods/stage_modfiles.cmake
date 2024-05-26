cmake_minimum_required(VERSION 3.5...3.27)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Expect the script to be called from the directory in which output should be placed (i.e. ${CMAKE_CURRENT_BINARY_DIR} from the caller)
# - INPUT_PATH: the full path for the input mod (i.e. the folder containing the mod files)
# - STAGING_FILES_DIRECTORY: the staging directory into which most files for this mod will be copied
# - STAGING_INFO_DIRECTORY: the staging directory into which info files (mod-info.json, mod-banner.png) will be copied

if(NOT DEFINED INPUT_PATH OR "${INPUT_PATH}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: INPUT_PATH" )
endif()
if(NOT DEFINED STAGING_FILES_DIRECTORY OR "${STAGING_FILES_DIRECTORY}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: STAGING_FILES_DIRECTORY" )
endif()
if(NOT DEFINED STAGING_INFO_DIRECTORY OR "${STAGING_INFO_DIRECTORY}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: STAGING_INFO_DIRECTORY" )
endif()

#################################

# Create empty staging directories
message(STATUS "STAGING_FILES_DIRECTORY=${STAGING_FILES_DIRECTORY}")
message(STATUS "STAGING_INFO_DIRECTORY=${STAGING_INFO_DIRECTORY}")

if(EXISTS "${STAGING_FILES_DIRECTORY}")
	file(REMOVE_RECURSE "${STAGING_FILES_DIRECTORY}/")
endif()
if(EXISTS "${STAGING_INFO_DIRECTORY}")
	file(REMOVE_RECURSE "${STAGING_INFO_DIRECTORY}/")
endif()
file(MAKE_DIRECTORY "${STAGING_FILES_DIRECTORY}")
file(MAKE_DIRECTORY "${STAGING_INFO_DIRECTORY}")

# Stage the mod files in STAGING_FILES_DIRECTORY (minus the mod-info.json + mod-banner.png)
file(COPY "${INPUT_PATH}/" DESTINATION "${STAGING_FILES_DIRECTORY}"
	PATTERN ".git*" EXCLUDE
	PATTERN ".DS_Store" EXCLUDE
	PATTERN "mod-info.json" EXCLUDE
	PATTERN "mod-banner.png" EXCLUDE
)

# Stage the mod info in STAGING_INFO_DIRECTORY (just the mod-info.json + mod-banner.png)
file(GLOB _info_files LIST_DIRECTORIES false
	"${INPUT_PATH}/mod-info.json"
	"${INPUT_PATH}/mod-banner.png"
)
file(COPY ${_info_files} DESTINATION "${STAGING_INFO_DIRECTORY}")
