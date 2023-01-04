# - Try to find the Opus library
# Once done this will define
#
#  OPUS_FOUND - system has Opus
#  OPUS_INCLUDE_DIR - the OggOpus include directory
#  OPUS_LIBRARY      - The Opus library
#
# Also creates the imported Opus::opus target

find_path(OPUS_INCLUDE_DIR opus/opus.h)
find_library(OPUS_LIBRARY NAMES opus)

mark_as_advanced(OPUS_INCLUDE_DIR OPUS_LIBRARY)

add_library(Opus::opus UNKNOWN IMPORTED)
set_target_properties(Opus::opus
  PROPERTIES
  IMPORTED_LOCATION "${OPUS_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${OPUS_INCLUDE_DIR}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus REQUIRED_VARS OPUS_LIBRARY OPUS_INCLUDE_DIR)
