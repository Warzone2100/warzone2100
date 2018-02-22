cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

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

set(CPACK_PACKAGE_NAME			"Warzone 2100")
set(CPACK_PACKAGE_VENDOR		"Warzone 2100 Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_CHECKSUM SHA256)

# Default components that are added to package
set(CPACK_COMPONENTS_ALL Core Languages)

if(NOT DEFINED CPACK_MODULE_PATH)
	set(CPACK_MODULE_PATH)
endif()

if(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
	# Default of "warzone2100"
	set(CPACK_PACKAGE_FILE_NAME "warzone2100")
endif()

if(NOT DEFINED CPACK_SOURCE_PACKAGE_FILE_NAME)
	# Default of "warzone2100"
	set(CPACK_SOURCE_PACKAGE_FILE_NAME "warzone2100")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# When targeting Windows, build the NSIS installer

	set(CPACK_GENERATOR "NSIS")

	set(INSTALLER_EXTDIR "" CACHE STRING "NSIS ext dir")
	set(INSTALLER_COMPRESSION 1 CACHE STRING "NSIS compression level")

	# Retrieve the warzone2100 executable file output_name
	# (This is set by src/CMakeLists.txt)
	get_target_property(_warzone_target_output_name warzone2100 OUTPUT_NAME)
	if("${_warzone_target_output_name}" STREQUAL "NOTFOUND")
		message( FATAL_ERROR "Could not retrieve the OUTPUT_NAME from the warzone2100 target. Is this file being included after the file that creates the target?" )
	endif()

	set(PACKAGE "${_warzone_target_output_name}")
	if(WZ_PORTABLE)
		set(PACKAGE_NAME "Warzone 2100 Portable")
	else()
		set(PACKAGE_NAME "Warzone 2100")
	endif()
	set(CPACK_NSIS_DISPLAY_NAME PACKAGE_NAME)
	set(CPACK_NSIS_PACKAGE_NAME PACKAGE_NAME)

	set(PORTABLE WZ_PORTABLE)
	set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
	set(BUILD_DIR "${CMAKE_BINARY_DIR}")

	set(CPACK_COMPONENT_CORE_DISPLAY_NAME "$(TEXT_SecBase)")
	set(CPACK_COMPONENT_CORE_REQUIRED ON)
	set(CPACK_COMPONENT_CORE_DESCRIPTION "$(DESC_SecBase)")

	set(CPACK_COMPONENT_LANGUAGES_DISPLAY_NAME "$(TEXT_SecNLS)")
	set(CPACK_COMPONENT_LANGUAGES_DESCRIPTION "$(DESC_SecNLS)")

	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/nsis/NSIS.definitions.in
		${CMAKE_BINARY_DIR}/NSIS.definitions.nsh
	)

	# IMPORTANT: To generate the NSIS script, CPack must use `nsis/NSIS.template.in`
	list(INSERT CPACK_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/nsis")

	# NSIS installer should always have component installation enabled
	set(CPACK_NSIS_COMPONENT_INSTALL ON)

endif()

#####################
# Package Source
set(CPACK_SOURCE_GENERATOR "TXZ")

if(NOT DEFINED CPACK_SOURCE_IGNORE_FILES)
	set (CPACK_SOURCE_IGNORE_FILES)
endif()

# Ignore version control
list (APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.#")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/#")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.git")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.svn")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.hg/")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.bzr/")

# Ignore development / runtime created files
list (APPEND CPACK_SOURCE_IGNORE_FILES "~$")
list (APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.DS_Store$")

# The build directory (if user configured to build in the core repo folder)
list (APPEND CPACK_SOURCE_IGNORE_FILES "/build/")

# Ignore sequences.wz if it's in the data directory
list (APPEND CPACK_SOURCE_IGNORE_FILES "/data/sequences.wz")

# Ignore any autorevision.cache file in the src directory
# (the install script below handles generating a fresh one)
list (APPEND CPACK_SOURCE_IGNORE_FILES "/src/autorevision.cache")

######################
# Configure Installer Version-info at Package Build-Time

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CPackConfigVersion.cmake.in
			   ${CMAKE_BINARY_DIR}/CPackConfigVersion.cmake
			   @ONLY)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_BINARY_DIR}/CPackConfigVersion.cmake)

######################
# Handling Autorevision for Package Source

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/autorevisionForSourcePackage.cmake.in
			   ${CMAKE_BINARY_DIR}/autorevisionForSourcePackage.cmake
			   @ONLY)
set(CPACK_INSTALL_SCRIPT "${CMAKE_BINARY_DIR}/autorevisionForSourcePackage.cmake")

INCLUDE(CPack)

