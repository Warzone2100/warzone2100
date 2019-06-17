cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0054 NEW)

#########################
# Generator-specific components

install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}${WZ_OUTPUT_NAME_SUFFIX}"
  COMPONENT DebianInfo
)

#########################

function(copyFileToDir _file _destDir)
	get_filename_component(_filename "${_file}" NAME)
	configure_file(
		${_file}
		${_destDir}/${_filename}
		COPYONLY
	)
endfunction()

if(NOT DEFINED WZ_DIST_LICENSE OR NOT EXISTS "${WZ_DIST_LICENSE}")
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DIST_LICENSE is defined.")
endif()

set(CPACK_PACKAGE_NAME			"Warzone 2100")
set(CPACK_PACKAGE_VENDOR		"Warzone 2100 Project")
set(CPACK_RESOURCE_FILE_LICENSE "${WZ_DIST_LICENSE}")
set(CPACK_PACKAGE_CHECKSUM		SHA256)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Description.txt")
set(CPACK_PACKAGE_HOMEPAGE_URL  "https://wz2100.net")

#########################
# Default components that are added to package

set(CPACK_COMPONENTS_ALL Core Data Docs Info Languages Fonts)
if(UNIX AND NOT APPLE)
	list(APPEND CPACK_COMPONENTS_ALL Manpages)
endif()
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

set(CPACK_COMPONENT_MANPAGES_GROUP "CoreGroup")

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

		# For the portable build, install `launch_warzone.bat` in the base folder
		install(FILES
			"${CMAKE_CURRENT_SOURCE_DIR}/nsis/launch_warzone.bat"
			COMPONENT Core DESTINATION "."
		)
	else()
		set(PACKAGE_NAME "Warzone 2100")
	endif()
	set(CPACK_NSIS_DISPLAY_NAME PACKAGE_NAME)
	set(CPACK_NSIS_PACKAGE_NAME PACKAGE_NAME)

	set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
	set(BUILD_DIR "${CMAKE_BINARY_DIR}")

	if(NOT DEFINED WZ_DATADIR)
		message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
	endif()
	if(NOT WZ_DATADIR_ISABSOLUTE)
		set(NSIS_WZ_DATADIR_APPEND "${WZ_DATADIR}")
		# NSIS expects backslashes
		string(REGEX REPLACE "/" "\\\\" NSIS_WZ_DATADIR_APPEND "${NSIS_WZ_DATADIR_APPEND}")
	else()
		message(FATAL_ERROR "NSIS installer does not support absolute WZ_DATADIR")
	endif()

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

	# IMPORTANT: To generate the NSIS script, CPack must use `nsis/NSIS.template.in`
	list(INSERT CPACK_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/nsis")

	# NSIS installer should always have component installation enabled
	set(CPACK_NSIS_COMPONENT_INSTALL ON)

	# Additional information
	set(CPACK_NSIS_HELP_LINK "https://wz2100.net")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://wz2100.net")

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# macOS builds

	# Only install the "Core" component (the application), as the macOS app bundle contains all of the data & supporting files & languages
	set(CPACK_COMPONENTS_ALL Core)
	unset(CPACK_COMPONENT_CORE_DEPENDS)
	set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

	# Default package generators on macOS
	set(CPACK_GENERATOR "ZIP")

	# Support Archive generator
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)

	# Support DragNDrop generator
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/dmg/WZDMGBackground.tiff")
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/dmg/WZDMGSetup.scpt")
	set(CPACK_DMG_VOLUME_NAME "Warzone 2100")

endif()

#####################
# RPM Settings

set(CPACK_RPM_PACKAGE_SUMMARY   "Innovative 3D real-time strategy")
set(CPACK_RPM_PACKAGE_GROUP     "Amusements/Games")
set(CPACK_RPM_PACKAGE_LICENSE   "GPLv2+ and CC-BY-SA")
set(CPACK_RPM_PACKAGE_URL       "https://wz2100.net/")

#####################
# DEB Settings

set(CPACK_DEBIAN_PACKAGE_NAME "warzone2100")
set(CPACK_DEBIAN_PACKAGE_SECTION  "games")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://wz2100.net/")

set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Innovative 3D real-time strategy\n In Warzone 2100 you command the forces of “The Project” in a battle to rebuild the world after mankind has been nearly destroyed by nuclear missiles.\n .\n The game offers campaign, tutorial, multi-player and single-player skirmish modes. An extensive tech tree with over 400 different technologies, combined with the unit design system, allows for a wide variety of possible units and tactics. Play versus AI bots in local skirmishes (or as added teammates / enemies in multiplayer), and practice your skills with endless replayability.\n .\n Warzone 2100 was originally created by Pumpkin Studios and published by Eidos Interactive, and was open-sourced in 2004. It has been developed, maintained, and improved by the community ever since, under the banner of the “Warzone 2100 Project”.\n .\n This package does not include the campaign video sequences - please see the Warzone 2100 website for details on how to download and install them.")

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

# Ignore various CI / Github-config related files
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.ci/")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.github/")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.cirrus.yml")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.lgtm.yml")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.travis.yml")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\appveyor.yml")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\azure-pipelines.yml")
list (APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\crowdin.yml")

# Ignore development / runtime created files
list (APPEND CPACK_SOURCE_IGNORE_FILES "~$")
list (APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.DS_Store$")

# The build directory (if user configured to build in the core repo folder)
list (APPEND CPACK_SOURCE_IGNORE_FILES "/build/")

# Ignore sequences.wz if it's in the data directory
list (APPEND CPACK_SOURCE_IGNORE_FILES "/data/sequences.wz")

# Ignore any autorevision.cache file in the src directory
# (the install script below handles generating a fresh one)
list (APPEND CPACK_SOURCE_IGNORE_FILES "/build_tools/autorevision.cache")

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

