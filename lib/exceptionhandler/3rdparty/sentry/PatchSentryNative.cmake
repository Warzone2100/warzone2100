if(NOT CMAKE_SCRIPT_MODE_FILE)
 message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_scriptFolder "${_fullPathToThisScript}" DIRECTORY)

if((NOT DEFINED SOURCE_DIR) OR (SOURCE_DIR STREQUAL ""))
 message(FATAL_ERROR "SOURCE_DIR must be specified on command-line")
endif()

# Fix sentry_sync.h x86 compile on llvm-mingw
set(_sentry_sync_h_path "${SOURCE_DIR}/src/sentry_sync.h")
set(_sentry_sync_find_text "#if defined(__MINGW32__) && !defined(__MINGW64__)")
set(_sentry_sync_replace_text "#if defined(__MINGW32__) && !defined(__MINGW64__) && !defined(__clang__)")
file(READ "${_sentry_sync_h_path}" FILE_CONTENTS)
string(REPLACE "${_sentry_sync_find_text}" "${_sentry_sync_replace_text}" FILE_CONTENTS "${FILE_CONTENTS}")
file(WRITE "${_sentry_sync_h_path}" "${FILE_CONTENTS}")
