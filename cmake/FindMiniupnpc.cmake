# - Locate Miniupnpc
#
# This module defines:
#
#  MINIUPNPC_INCLUDE_DIR
#  MINIUPNPC_LIBRARY
#  MINIUPNPC_FOUND
#  MINIUPNPC_API_VERSION_STRING, human-readable string containing the MINIUPNPC_API_VERSION
#
# If Miniupnpc is successfully detected, it also adds an IMPORTED library target: imported-miniupnpc
#
# To find Miniupnpc, specify:
#   find_package(Miniupnpc [MINIUPNPC_API_VERSION] [REQUIRED])
#
# NOTES:
# - If a "version" is specified in the find_package command, it will be compared against the MINIUPNPC_API_VERSION,
#   *NOT* the MINIUPNPC_VERSION.
# - MINIUPNPC_API_VERSION was added in MINIUPNPC_VERSION 1.7. Version detection for earlier Miniupnpc releases that
#   lack the MINIUPNPC_API_VERSION define is not currently supported.
#

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(_MINIUPNPC_PKGCONFIG QUIET libminiupnpc)
endif()

find_path(MINIUPNPC_INCLUDE_DIR NAMES miniupnpc/miniupnpc.h HINTS ${_MINIUPNPC_PKGCONFIG_INCLUDEDIR})
find_library(MINIUPNPC_LIBRARY NAMES miniupnpc libminiupnpc HINTS ${_MINIUPNPC_PKGCONFIG_LIBDIR})

if(MINIUPNPC_INCLUDE_DIR AND EXISTS "${MINIUPNPC_INCLUDE_DIR}/miniupnpc/miniupnpc.h")
	# Extract MINIUPNPC_API_VERSION from miniupnpc.h
	file(STRINGS "${MINIUPNPC_INCLUDE_DIR}/miniupnpc/miniupnpc.h" MINIUPNPC_API_VERSION_LINE REGEX "^#define[ \t]+MINIUPNPC_API_VERSION[ \t]+[0-9]+")
	if(MINIUPNPC_API_VERSION_LINE)
		string(REGEX REPLACE "^#define[ \t]+MINIUPNPC_API_VERSION[ \t]+([0-9]+)" "\\1" MINIUPNPC_API_VERSION_STRING "${MINIUPNPC_API_VERSION_LINE}")
	else()
		set(MINIUPNPC_API_VERSION_STRING "")
	endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
	Miniupnpc
	REQUIRED_VARS MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY
	VERSION_VAR MINIUPNPC_API_VERSION_STRING
)

if(MINIUPNPC_FOUND)
	add_library(imported-miniupnpc UNKNOWN IMPORTED)
	set_target_properties(imported-miniupnpc
		PROPERTIES
		IMPORTED_LOCATION ${MINIUPNPC_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${MINIUPNPC_INCLUDE_DIR}
	)
endif()

mark_as_advanced(MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY)
