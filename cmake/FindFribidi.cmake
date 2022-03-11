# Locate fribidi
# This module defines:
#
#  FRIBIDI_INCLUDE_DIRS
#  FRIBIDI_LIBRARIS
#  FRIBIDI_FOUND
#  FRIBIDI_VERSION_STRING
#

find_package(PkgConfig)
# If PkgConfig is available, attempt to find Fribidi paths using PkgConfig
if(PkgConfig_FOUND)
	if(Fribidi_FIND_VERSION)
		if(Fribidi_FIND_VERSION_EXACT)
			set(_pkgConfig_version_match "==${Fribidi_FIND_VERSION}")
		else()
			set(_pkgConfig_version_match ">=${Fribidi_FIND_VERSION}")
		endif()
	endif()

	pkg_search_module(PCFG_FRIBIDI QUIET fribidi${_pkgConfig_version_match})

	if(PCFG_FRIBIDI_FOUND)
		# Found Fribidi with PkgConfig
		message(STATUS "Detected Fribidi with PkgConfig: FRIBIDI_INCLUDE_DIRS (${PCFG_FRIBIDI_INCLUDE_DIRS}); FRIBIDI_LIBRARY_DIRS (${PCFG_FRIBIDI_LIBRARY_DIRS})")
	endif()
endif()

# Attempt to find Fribidi

find_path(
	FRIBIDI_INCLUDE_DIRS fribidi.h
	PATH_SUFFIXES fribidi
	HINTS ${PCFG_FRIBIDI_INCLUDE_DIRS} ${PCFG_FRIBIDI_INCLUDEDIR}
)

find_library(
	FRIBIDI_LIBRARIES NAMES fribidi
	HINTS ${PCFG_FRIBIDI_LIBRARY_DIRS} ${PCFG_FRIBIDI_LIBDIR}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Fribidi REQUIRED_VARS FRIBIDI_INCLUDE_DIRS FRIBIDI_LIBRARIES VERSION_VAR FRIBIDI_VERSION_STRING)

mark_as_advanced(FRIBIDI_INCLUDE_DIRS FRIBIDI_LIBRARIES)
