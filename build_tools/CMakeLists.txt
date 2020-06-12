cmake_minimum_required(VERSION 3.5)

############################
# Versioning (autorevision)

# Autorevision target that will run *every* build
add_custom_target(autorevision ALL
    DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/__shouldnotexist.h"
)
set_property(TARGET autorevision PROPERTY FOLDER "_WZBuildProcessTargets")

# Autorevision command that will run *every* build
set(_autorevision_cache_tarball_src_input "${CMAKE_CURRENT_SOURCE_DIR}/autorevision.cache")
set(_autorevision_cache_file "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache")
set(_autorevision_h_file "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h")
add_custom_command(
    OUTPUT
		"${CMAKE_CURRENT_BINARY_DIR}/__shouldnotexist.h"  # fake - ensure we run
		"${_autorevision_h_file}"
		"${_autorevision_cache_file}"
	# Copy autorevision.cache from the source build_tools directory (if present)
	# This supports proper versioning from distribution source archives
	COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${_autorevision_cache_tarball_src_input} -DOUTPUT_FILE=${_autorevision_cache_file} -DSKIP_IF_INPUT_MISSING=ON -DSKIP_IF_OUTPUT_EXISTS=ON -P ${CMAKE_SOURCE_DIR}/cmake/CopyFileHelper.cmake
	# Autorevision.cmake
	# this command must generate: ${CMAKE_CURRENT_BINARY_DIR}/autorevision.h
	COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${_autorevision_cache_file} -DOUTPUT_TYPE=h -DOUTPUT_FILE=${_autorevision_h_file} -P ${CMAKE_SOURCE_DIR}/build_tools/autorevision.cmake
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	VERBATIM
)

set(wz2100_autorevision_cache_file "${_autorevision_cache_file}" PARENT_SCOPE)
set(wz2100_autorevision_h_file "${_autorevision_h_file}" PARENT_SCOPE)

# IMPORTANT: If using the above files in other scopes / directories, the GENERATED property must be set in each CMakeLists.txt
# Example:
#	set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
# 	set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)
