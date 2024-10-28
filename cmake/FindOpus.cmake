# - Try to find the Opus library
# Once done this will define
#
#  Opus_FOUND - system has Opus
#  Opus_INCLUDE_DIR - the OggOpus include directory
#  Opus_LIBRARY      - The Opus library
#
# Also creates the imported Opus::opus target

# Try config mode first!
find_package(Opus CONFIG QUIET) # Deliberately quiet, so we can handle the result
if(Opus_FOUND)
	if (TARGET Opus::opus)
		# CONFIG mode succeeded
		if(NOT Opus_INCLUDE_DIR)
			get_target_property(Opus_INCLUDE_DIR Opus::opus INTERFACE_INCLUDE_DIRECTORIES)
		endif()
		if(NOT Opus_LIBRARY)
			set(Opus_LIBRARY Opus::opus)
		endif()
		message(STATUS "Found Opus: ${Opus_INCLUDE_DIR}")
		return()
	endif()
endif()

find_path(Opus_INCLUDE_DIR opus/opus.h)
find_library(Opus_LIBRARY NAMES opus)

mark_as_advanced(Opus_INCLUDE_DIR Opus_LIBRARY)

add_library(Opus::opus UNKNOWN IMPORTED)
set_target_properties(Opus::opus
  PROPERTIES
  IMPORTED_LOCATION "${Opus_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${Opus_INCLUDE_DIR}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus REQUIRED_VARS Opus_LIBRARY Opus_INCLUDE_DIR)
