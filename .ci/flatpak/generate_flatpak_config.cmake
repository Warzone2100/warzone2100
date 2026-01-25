cmake_minimum_required(VERSION 3.16...3.31)

# Automatically generate a Flatpak manifest from the template file
#
# Required input defines:
# - TEMPLATE_FILE: the full filename + path for the input net.wz2100.wz2100.yaml.in template file
# - OUTPUT_FILE: the full filename + path for the output net.wz2100.wz2100.yaml file
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
#
# And also, these input defines:
# - WZ_OUTPUT_NAME_SUFFIX: The desired suffix to apply to the app id in the manifest (should match the main build)
#
# If cross-compiling, specify:
# - WZ_CROSS_COMPILE_TARGET_ARCH: <aarch64, etc>
#

if(NOT DEFINED TEMPLATE_FILE OR "${TEMPLATE_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: TEMPLATE_FILE" )
endif()
if(NOT DEFINED OUTPUT_FILE OR "${OUTPUT_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: OUTPUT_FILE" )
endif()
if(NOT DEFINED PROJECT_ROOT OR "${PROJECT_ROOT}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: PROJECT_ROOT" )
endif()

if(NOT DEFINED WZ_OUTPUT_NAME_SUFFIX)
	message( FATAL_ERROR "Missing expected input define: WZ_OUTPUT_NAME_SUFFIX" )
endif()
if (DEFINED WZ_CROSS_COMPILE_TARGET_ARCH)
	message( FATAL_ERROR "WZ_CROSS_COMPILE_TARGET_ARCH detected - not currently supported by this script" )
endif()

if (WZ_RELEASE_PUBLISH_BUILD)
	if(NOT DEFINED WZ_RELEASE_TAG OR "${WZ_RELEASE_TAG}" STREQUAL "")
		message( FATAL_ERROR "Missing expected input define: WZ_RELEASE_TAG" )
	endif()
	if(NOT DEFINED WZ_RELEASE_TARBALL_SHA256 OR "${WZ_RELEASE_TARBALL_SHA256}" STREQUAL "")
		message( FATAL_ERROR "Missing expected input define: WZ_RELEASE_TARBALL_SHA256" )
	endif()
endif()

get_filename_component(_input_dir "${TEMPLATE_FILE}" DIRECTORY)

##################################

if (NOT WZ_RELEASE_PUBLISH_BUILD)
	# Default WZ module source (for regular CI builds, the current dir into which the Github repo is checked-out)
	# Assumes the generated config will be placed in .ci/flatpak/
	set(WZ_MAIN_MODULE_SOURCE "\n\
      - type: dir\n\
        path: ../../\n\
")

else()
	# For release-publishing-triggered builds (i.e. for Flathub), use the .tar.xz from the release
	message( STATUS "WZ_RELEASE_PUBLISH_BUILD detected - configuring for build from source tarball for \"${WZ_RELEASE_TAG}\"" )
	set(WZ_MAIN_MODULE_SOURCE "
      - type: archive\n\
        url: https://github.com/Warzone2100/warzone2100/releases/download/${WZ_RELEASE_TAG}/warzone2100_src.tar.xz\n\
        sha256: ${WZ_RELEASE_TARBALL_SHA256}\n\
")

endif()

##################################
# Handle sentry-native

# Get the source URL AND SHA512 from a data file
set(_sentry_dl_data_file "${PROJECT_ROOT}/.sentrynative")
file(STRINGS "${_sentry_dl_data_file}" _sentry_native_url_info ENCODING UTF-8)
while(_sentry_native_url_info)
	list(POP_FRONT _sentry_native_url_info LINE)
	if (LINE MATCHES "^URL=(.*)")
		set(WZ_SENTRY_NATIVE_URL "${CMAKE_MATCH_1}")
	endif()
	if (LINE MATCHES "^SHA512=(.*)")
		set(WZ_SENTRY_NATIVE_SHA512 "${CMAKE_MATCH_1}")
	endif()
endwhile()
unset(_sentry_native_url_info)
if(NOT DEFINED WZ_SENTRY_NATIVE_URL OR NOT DEFINED WZ_SENTRY_NATIVE_SHA512)
	message(FATAL_ERROR "Failed to load URL and hash from: ${_sentry_dl_data_file}")
endif()
unset(_sentry_dl_data_file)

##################################
# Handle prebuilt-texture-packages

include("${PROJECT_ROOT}/data/WZPrebuiltPackages.cmake")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++TEMPLATE_FILE: ${TEMPLATE_FILE}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++OUTPUT_FILE: ${OUTPUT_FILE}")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_OUTPUT_NAME_SUFFIX: ${WZ_OUTPUT_NAME_SUFFIX}")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_SENTRY_NATIVE_URL: ${WZ_SENTRY_NATIVE_URL}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_SENTRY_NATIVE_SHA512: ${WZ_SENTRY_NATIVE_SHA512}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

