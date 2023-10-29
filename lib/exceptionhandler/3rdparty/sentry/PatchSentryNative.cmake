if(NOT CMAKE_SCRIPT_MODE_FILE)
 message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_scriptFolder "${_fullPathToThisScript}" DIRECTORY)

if((NOT DEFINED SOURCE_DIR) OR (SOURCE_DIR STREQUAL ""))
 message(FATAL_ERROR "SOURCE_DIR must be specified on command-line")
endif()

# Remove compat/mingw/werapi.h (no longer needed)
if (EXISTS "${SOURCE_DIR}/external/crashpad/compat/mingw/werapi.h")
	file(REMOVE "${SOURCE_DIR}/external/crashpad/compat/mingw/werapi.h")
endif()
