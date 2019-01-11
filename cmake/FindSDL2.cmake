# - Try to locate SDL2
# This module defines:
#
#  SDL2_INCLUDE_DIR
#  SDL2_LIBRARY
#  SDL2_FOUND
#  SDL2_VERSION_STRING, human-readable string containing the version of SDL
#
# Portions derived from FindSDL.cmake, distributed under the OSI-approved BSD 3-Clause License (https://cmake.org/licensing)
#

find_path(SDL2_INCLUDE_DIR NAMES SDL.h PATH_SUFFIXES SDL2)

find_library(SDL2_LIBRARY_RELEASE NAMES SDL2)
find_library(SDL2_LIBRARY_DEBUG NAMES SDL2d)

find_library(SDL2MAIN_LIBRARY_RELEASE NAMES SDL2main)
find_library(SDL2MAIN_LIBRARY_DEBUG NAMES SDL2maind)

include(SelectLibraryConfigurations)
select_library_configurations(SDL2)
select_library_configurations(SDL2MAIN)

mark_as_advanced(SDL2_LIBRARY_RELEASE SDL2_LIBRARY_DEBUG)
mark_as_advanced(SDL2MAIN_LIBRARY_RELEASE SDL2MAIN_LIBRARY_DEBUG)

if (SDL2_INCLUDE_DIR AND EXISTS "${SDL2_INCLUDE_DIR}/SDL_version.h")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_INCLUDE_DIR}/SDL_version.h" SDL2_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MAJOR "${SDL2_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_MINOR "${SDL2_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_VERSION_PATCH "${SDL2_VERSION_PATCH_LINE}")
  set(SDL2_VERSION_STRING ${SDL2_VERSION_MAJOR}.${SDL2_VERSION_MINOR}.${SDL2_VERSION_PATCH})
  unset(SDL2_VERSION_MAJOR_LINE)
  unset(SDL2_VERSION_MINOR_LINE)
  unset(SDL2_VERSION_PATCH_LINE)
  unset(SDL2_VERSION_MAJOR)
  unset(SDL2_VERSION_MINOR)
  unset(SDL2_VERSION_PATCH)
else()
  if (SDL2_INCLUDE_DIR)
    message ( WARNING "Can't find ${SDL2_INCLUDE_DIR}/SDL_version.h" )
  endif()
  set(SDL2_VERSION_STRING "")
endif()

# Set by select_library_configurations(), but we want the one from
# find_package_handle_standard_args() below.
unset(SDL2_FOUND)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 REQUIRED_VARS SDL2_INCLUDE_DIR SDL2_LIBRARY VERSION_VAR SDL2_VERSION_STRING)

set(SDL2MAIN_LIBRARY "${SDL2MAIN_LIBRARY}" CACHE STRING "Where the SDL2main Library can be found")

mark_as_advanced(SDL2_INCLUDE_DIR SDL2_LIBRARY SDL2MAIN_FOUND SDL2MAIN_LIBRARY)
