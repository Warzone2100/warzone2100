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

# Patch compat/mingw files
execute_process(
   COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/crashpad/dbghelp.h" "${SOURCE_DIR}/external/crashpad/compat/mingw/dbghelp.h"
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
 )
execute_process(
   COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/crashpad/winnt.h" "${SOURCE_DIR}/external/crashpad/compat/mingw/winnt.h"
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
 )
execute_process(
   COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/crashpad/werapi.h" "${SOURCE_DIR}/external/crashpad/compat/mingw/werapi.h"
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
 )

# Patch for mingw compilation
execute_process(
   COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/crashpad/snapshot/CMakeLists.txt" "${SOURCE_DIR}/external/crashpad/snapshot/CMakeLists.txt"
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
 )
execute_process(
   COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/src/backends/sentry_backend_crashpad.cpp" "${SOURCE_DIR}/src/backends/sentry_backend_crashpad.cpp"
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
 )


