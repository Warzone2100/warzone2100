# - Try to find the Opusfile library
# Once done this will define:
#
#  OPUSFILE_FOUND - system has Opusfile
#  OPUSFILE_INCLUDE_DIR - the Opusfile include directory
#  OPUSFILE_LIBRARY  - The Opusfile library
#
# Plus the Opus::opusfile target
#

include (CheckLibraryExists)

find_path(OPUSFILE_INCLUDE_DIR opusfile.h PATH_SUFFIX opus)

# opusfile.h includes "opus_multistream.h" instead of "opus/opus_multistream.h"
# so we must include the path to opus_multistream.h
find_path(OPUS_MULTISTREAM_DIR opus_multistream.h PATH_SUFFIX opus)

find_library(OPUSFILE_LIBRARY NAMES opusfile)
if (NOT TARGET Ogg::ogg)
	find_package(Ogg REQUIRED)
endif()
if (NOT TARGET Opus::opus)
	find_package(Opus REQUIRED)
endif()

mark_as_advanced(OPUSFILE_INCLUDE_DIR OPUSFILE_LIBRARY)

list(APPEND _opusfile_interface_include_directories "${OPUSFILE_INCLUDE_DIR}")
list(APPEND _opusfile_interface_include_directories "${OPUS_MULTISTREAM_DIR}")
list(APPEND _opusfile_link_libraries "Ogg::ogg" "Opus::opus")
add_library(Opus::opusfile UNKNOWN IMPORTED)
set_target_properties(Opus::opusfile
  PROPERTIES
  IMPORTED_LOCATION "${OPUSFILE_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${_opusfile_interface_include_directories}"
  INTERFACE_LINK_LIBRARIES "${_opusfile_link_libraries}"
)
unset(_opusfile_interface_include_directories)
unset(_opusfile_link_libraries)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opusfile REQUIRED_VARS OPUSFILE_LIBRARY OPUSFILE_INCLUDE_DIR)
