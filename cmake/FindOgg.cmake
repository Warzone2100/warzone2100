# - Try to find the Ogg library
# Once done this will define
#
#  OGG_FOUND - system has OggOpus
#  OGG_INCLUDE_DIR - the OggOpus include directory
#  OGG_LIBRARY        - The Ogg library
#
# Also creates the imported Ogg::ogg target

find_path(OGG_INCLUDE_DIR  ogg/ogg.h)
find_library(OGG_LIBRARY NAMES ogg)

mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARY)

add_library(Ogg::ogg UNKNOWN IMPORTED)
set_target_properties(Ogg::ogg
  PROPERTIES
  IMPORTED_LOCATION "${OGG_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg REQUIRED_VARS OGG_LIBRARY OGG_INCLUDE_DIR)
