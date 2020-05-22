# - Try to locate libsodium
# This module defines:
#
#  sodium_FOUND - true if sodium found on the system
#  sodium_VERSION, human-readable string containing the version of sodium
#
# and a target:
#  unofficial-sodium::sodium
#

include(FindPackageHandleStandardArgs)

if (VCPKG_TOOLCHAIN)
	find_package(unofficial-sodium CONFIG QUIET)
	# If we found the vcpkg unofficial-sodium configuration, return with that result
	if(unofficial-sodium_FOUND)
		find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME} REQUIRED_VARS sodium_INCLUDE_DIR sodium_LIBRARIES VERSION_VAR sodium_VERSION)
		message(STATUS "Using vcpkg unofficial-sodium configuration (${sodium_VERSION})")
		return()
	else()
		message(STATUS "Did not find vcpkg unofficial-sodium configuration, reverting to regular check")
	endif()
endif()

find_package(PkgConfig)
# If PkgConfig is available, attempt to find Sodium paths using PkgConfig
if(PkgConfig_FOUND)
	if(Sodium_FIND_VERSION)
		if(Sodium_FIND_VERSION_EXACT)
			set(_pkgConfig_version_match "==${Sodium_FIND_VERSION}")
		else()
			set(_pkgConfig_version_match ">=${Sodium_FIND_VERSION}")
		endif()
	endif()
	pkg_search_module(PCFG_SODIUM QUIET libsodium${_pkgConfig_version_match})
	if(PCFG_SODIUM_FOUND)
		# Found Sodium with PkgConfig
		message(STATUS "Detected libsodium with PkgConfig: SODIUM_INCLUDE_DIRS (${PCFG_SODIUM_INCLUDE_DIRS}); SODIUM_LIBRARY_DIRS (${PCFG_SODIUM_LIBRARY_DIRS})")
	endif()
endif()

# Attempt to find libsodium

FIND_PATH(
	sodium_INCLUDE_DIR sodium.h
	HINTS ${PCFG_SODIUM_INCLUDE_DIRS} ${PCFG_SODIUM_INCLUDEDIR}
)
FIND_LIBRARY(
	sodium_LIBRARIES NAMES libsodium sodium
	HINTS ${PCFG_SODIUM_LIBRARY_DIRS} ${PCFG_SODIUM_LIBDIR}
)

if(sodium_INCLUDE_DIR AND EXISTS "${sodium_INCLUDE_DIR}/sodium/version.h")
	file(STRINGS "${sodium_INCLUDE_DIR}/sodium/version.h" SODIUM_VERSION_STRING_LINE REGEX "^#define[ \t]+SODIUM_VERSION_STRING[ \t]+\"[.0-9]+\"$")
	string(REGEX REPLACE "^#define[ \t]+SODIUM_VERSION_STRING[ \t]+\"([.0-9]+)\"$" "\\1" sodium_VERSION "${SODIUM_VERSION_STRING_LINE}")
	unset(SODIUM_VERSION_STRING_LINE)
else()
	if (sodium_INCLUDE_DIR)
		message ( WARNING "Can't find ${sodium_INCLUDE_DIR}/sodium/version.h" )
	endif()
	set(sodium_VERSION "")
endif()

find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME} REQUIRED_VARS sodium_INCLUDE_DIR sodium_LIBRARIES VERSION_VAR sodium_VERSION)

# Create imported target unofficial-sodium::sodium
add_library(unofficial-sodium::sodium UNKNOWN IMPORTED)
set_target_properties(unofficial-sodium::sodium PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${sodium_INCLUDE_DIR}"
	IMPORTED_LOCATION "${sodium_LIBRARIES}"
)
