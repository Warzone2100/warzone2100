cmake_minimum_required(VERSION 3.5)

##################################################
# Run via: cmake -P apply_quickjs_patches.cmake
#

# Get full path to this file
if(NOT CMAKE_SCRIPT_MODE_FILE)
	message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_directoryOfThisScript "${_fullPathToThisScript}" DIRECTORY)

##################################################
# Derived from vcpkg_apply_patches at:
# https://github.com/microsoft/vcpkg/blob/master/scripts/cmake/vcpkg_apply_patches.cmake
##################################################
function(quickjs_apply_patches)
    # parse parameters such that semicolons in options arguments to COMMAND don't get erased
    cmake_parse_arguments(PARSE_ARGV 0 _ap "QUIET" "SOURCE_PATH" "PATCHES")

    find_package(Git REQUIRED)
    set(PATCHNUM 0)
    foreach(PATCH ${_ap_PATCHES})
        get_filename_component(ABSOLUTE_PATCH "${PATCH}" ABSOLUTE BASE_DIR "${_directoryOfThisScript}")
        message(STATUS "Applying patch ${PATCH}")
        set(LOGNAME patch-${TARGET_TRIPLET}-${PATCHNUM})
        execute_process(
            COMMAND ${GIT_EXECUTABLE} --work-tree=. apply "${ABSOLUTE_PATCH}" --ignore-whitespace --whitespace=nowarn --verbose
            OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-out.log
            ERROR_VARIABLE error
            WORKING_DIRECTORY ${_ap_SOURCE_PATH}
            RESULT_VARIABLE error_code
        )
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-err.log" "${error}")

        if(error_code)
			if(NOT _ap_QUIET)
				message(FATAL_ERROR "Applying patch failed. ${error}")
			else()
				message(WARNING "Applying patch failed - may have already been applied, or may require updating for a new release of QuickJS. See: ${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-err.log")
			endif()
        endif()

        math(EXPR PATCHNUM "${PATCHNUM}+1")
    endforeach()
endfunction()

##################################################

quickjs_apply_patches(
	QUIET
	SOURCE_PATH "${_directoryOfThisScript}/.."
	PATCHES
		"001-add-extensions.patch"
		"002-add-disable-atomics-define.patch"
		"003-fix-js-check-stack-overflow.patch"
		"004-msvc-compatibility.patch"
		"005-fix-pedantic-cxx-warnings.patch"
		"006-bsd-compile-fixes.patch"
		"007-msvc-64bit-compatibility.patch"
)

message(STATUS "Finished applying patches.")
