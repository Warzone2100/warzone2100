if(NOT CMAKE_SCRIPT_MODE_FILE)
	message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_scriptFolder "${_fullPathToThisScript}" DIRECTORY)

if((NOT DEFINED SOURCE_DIR) OR (SOURCE_DIR STREQUAL ""))
  message(FATAL_ERROR "SOURCE_DIR must be specified on command-line")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/CMakeLists.txt" "${SOURCE_DIR}/CMakeLists.txt"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "${_scriptFolder}/utf_string_conversion_utils.mingw.cc" "${SOURCE_DIR}/external/crashpad/third_party/mini_chromium/utf_string_conversion_utils.mingw.cc"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
