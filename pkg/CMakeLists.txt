cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0054 NEW)

function(copyFileToDir _file _destDir)
	get_filename_component(_filename "${_file}" NAME)
	configure_file(
		${_file}
		${_destDir}/${_filename}
		COPYONLY
	)
endfunction()

set(CPACK_PACKAGE_NAME			"Warzone 2100")
set(CPACK_PACKAGE_VENDOR		"Warzone 2100 Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_CHECKSUM		SHA256)

#########################
# Default components that are added to package

set(CPACK_COMPONENTS_ALL Core Data Docs Info Languages Fonts)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

#########################
# Default component info

set(CPACK_COMPONENT_CORE_DISPLAY_NAME "Application")
set(CPACK_COMPONENT_CORE_REQUIRED ON)
set(CPACK_COMPONENT_CORE_DESCRIPTION "Application")
set(CPACK_COMPONENT_CORE_GROUP "CoreGroup")
set(CPACK_COMPONENT_CORE_DEPENDS Data Docs Info)

set(CPACK_COMPONENT_DATA_DISPLAY_NAME "Data Files")
set(CPACK_COMPONENT_DATA_REQUIRED ON)
set(CPACK_COMPONENT_DATA_DESCRIPTION "Data Files")
set(CPACK_COMPONENT_DATA_GROUP "CoreGroup")

set(CPACK_COMPONENT_DOCS_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCS_REQUIRED ON)
set(CPACK_COMPONENT_DOCS_DESCRIPTION "Documentation")
set(CPACK_COMPONENT_DOCS_GROUP "CoreGroup")

set(CPACK_COMPONENT_INFO_DISPLAY_NAME "Info")
set(CPACK_COMPONENT_INFO_REQUIRED ON)
set(CPACK_COMPONENT_INFO_DESCRIPTION "Info Files")
set(CPACK_COMPONENT_INFO_GROUP "CoreGroup")

set(CPACK_COMPONENT_FONTS_DISPLAY_NAME "Fonts")
set(CPACK_COMPONENT_FONTS_REQUIRED ON)
set(CPACK_COMPONENT_FONTS_DESCRIPTION "Fonts")
set(CPACK_COMPONENT_FONTS_GROUP "CoreGroup")

set(CPACK_COMPONENT_GROUP_COREGROUP_DISPLAY_NAME "Core files")
set(CPACK_COMPONENT_GROUP_COREGROUP_REQUIRED ON)
set(CPACK_COMPONENT_GROUP_COREGROUP_DESCRIPTION "The core files required to run Warzone 2100.")

#########################

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

	set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
	set(BUILD_DIR "${CMAKE_BINARY_DIR}")

	# Localize component names & descriptions
	set(CPACK_COMPONENT_GROUP_COREGROUP_DISPLAY_NAME "$(TEXT_SecBase)")
	set(CPACK_COMPONENT_GROUP_COREGROUP_DESCRIPTION "$(DESC_SecBase)")
	set(CPACK_COMPONENT_LANGUAGES_DISPLAY_NAME "$(TEXT_SecNLS)")
	set(CPACK_COMPONENT_LANGUAGES_DESCRIPTION "$(DESC_SecNLS)")

	if(MSVC)
		set(NSIS_MSVCRUNTIME "on")
	else()
		unset(NSIS_MSVCRUNTIME)
	endif()

	# Note: NSIS.definitions.in also uses WZ_PORTABLE directly
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/nsis/NSIS.definitions.in
		${CMAKE_BINARY_DIR}/NSIS.definitions.nsh
	)

	# Copy Installer Language files
	copyFileToDir("${CMAKE_CURRENT_SOURCE_DIR}/nsis/Lang-English.nsh" "${CMAKE_BINARY_DIR}")
	copyFileToDir("${CMAKE_CURRENT_SOURCE_DIR}/nsis/Lang-Dutch.nsh" "${CMAKE_BINARY_DIR}")
	copyFileToDir("${CMAKE_CURRENT_SOURCE_DIR}/nsis/Lang-German.nsh" "${CMAKE_BINARY_DIR}")
	copyFileToDir("${CMAKE_CURRENT_SOURCE_DIR}/nsis/Lang-Russian.nsh" "${CMAKE_BINARY_DIR}")

	# IMPORTANT: To generate the NSIS script, CPack must use `nsis/NSIS.template.in`
	list(INSERT CPACK_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/nsis")

	# NSIS installer should always have component installation enabled
	set(CPACK_NSIS_COMPONENT_INSTALL ON)

	# Additional information
	set(CPACK_NSIS_HELP_LINK "http://wz2100.net")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://wz2100.net")

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# macOS builds

	# Only install the "Core" component (the application), as the macOS app bundle contains all of the data & supporting files & languages
	set(CPACK_COMPONENTS_ALL Core)
	set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

	# Default package generators on macOS
	set(CPACK_GENERATOR "ZIP;DragNDrop")

	# Support Archive generator
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)

	# Support DragNDrop generator
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/dmg/WZDMGBackground.tiff")
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/dmg/WZDMGSetup.scpt")
	set(CPACK_DMG_VOLUME_NAME "Warzone 2100")

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

# The macosx build directories
list (APPEND CPACK_SOURCE_IGNORE_FILES "/macosx/build/")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/macosx/external/.*/")

# Ignore in-source vcpkg directory
list (APPEND CPACK_SOURCE_IGNORE_FILES "/vcpkg/")

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

