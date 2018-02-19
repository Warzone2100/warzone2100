#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2018-02-18a
#

# ADD_TARGET_LINK_FLAGS_IF_SUPPORTED( TARGET <target>
#									  [COMPILER_TYPE <C | CXX> = CXX]
#									  LINK_FLAGS <link_flags>
#									  [CACHED_RESULT_NAME <cached_result_name>]
#									  [QUIET <ALL | FAILURES | OFF> = FAILURES] )
#
# ADD_TARGET_LINK_FLAGS_IF_SUPPORTED adds the specified link flags to the target if
# the link flags seem to be supported by the current (specified) compiler + linker.
#
# If COMPILER_TYPE C is specified, this checks if the configured C compiler + linker
# seems to support the link flags. If COMPILER_TYPE CXX is specified, this checks
# if the configured CXX compiler + linker seems to support the link flags. The
# default COMPILER_TYPE is CXX.
#
# If CACHED_RESULT_NAME is specified, the result of the link-flag check will be stored
# in a CACHE variable with the name ${CACHED_RESULT_NAME}. Every subsequent CMake run
# will re-use this cached value rather than performing the check again, even if the
# ``code`` changes. If you wish to perform the check anew on every CMake run, omit
# the CACHED_RESULT_NAME parameter. (In most cases, using a CACHED_RESULT_NAME is
# recommended.)
#
# QUIET can be specified to control message output with one of the following settings:
#	"ALL" - do NOT output any messages
#	"FAILIRES" - do NOT output messages on any failures
#	"OFF" - all output is enabled
#
function(ADD_TARGET_LINK_FLAGS_IF_SUPPORTED)
	set(_options)
	set(_oneValueArgs TARGET COMPILER_TYPE LINK_FLAGS CACHED_RESULT_NAME QUIET)
	set(_multiValueArgs)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _parsedArguments_TARGET)
		message( FATAL_ERROR "ADD_TARGET_LINK_FLAGS_IF_SUPPORTED requires TARGET" )
	endif()

	if(NOT DEFINED _parsedArguments_LINK_FLAGS)
		message( FATAL_ERROR "ADD_TARGET_LINK_FLAGS_IF_SUPPORTED requires LINK_FLAGS" )
	endif()

	if(DEFINED _parsedArguments_QUIET)
		if(NOT _parsedArguments_QUIET MATCHES "^(ALL|FAILURES|OFF)$")
			message( FATAL_ERROR "ADD_TARGET_LINK_FLAGS_IF_SUPPORTED invalid QUIET value - must be one of: (ALL, FAILURES, OFF)" )
		endif()
	else()
		set(_parsedArguments_QUIET "FAILURES")
	endif()

	if(NOT DEFINED _parsedArguments_COMPILER_TYPE)
		set(_parsedArguments_COMPILER_TYPE "CXX")
	endif()

	set(_check_cached_status)
	if(_parsedArguments_CACHED_RESULT_NAME)
		if(DEFINED "${_parsedArguments_CACHED_RESULT_NAME}")
			set(_check_cached_status " (cached)")
		endif()
		set(_cached_result_variable_name "${_parsedArguments_CACHED_RESULT_NAME}")
	else()
		# Generate a temporary variable name that isn't in the cache
		while(NOT _cached_result_variable_name)
			string(RANDOM LENGTH 12 _tmpRandom)
			set(_tmpRandom "_NOCACHE_tmp_rnd_${_tmpRandom}")
			if(NOT DEFINED "${_tmpRandom}")
				set(_cached_result_variable_name "${_tmpRandom}")
			endif()
		endwhile()
	endif()

	if(_parsedArguments_COMPILER_TYPE MATCHES "CXX")
		CHECK_CXX_LINKER_FLAGS("${_parsedArguments_LINK_FLAGS}" ${_cached_result_variable_name})
	elseif(_parsedArguments_COMPILER_TYPE MATCHES "C")
		CHECK_C_LINKER_FLAGS("${_parsedArguments_LINK_FLAGS}" ${_cached_result_variable_name})
	else()
		message( FATAL_ERROR "ADD_TARGET_LINK_FLAGS_IF_SUPPORTED invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
	endif()

	if(${_cached_result_variable_name})
		if(NOT _parsedArguments_QUIET OR _parsedArguments_QUIET MATCHES "FAILURES")
			message( STATUS "Set TARGET ${_parsedArguments_TARGET} LINK_FLAG: ${_parsedArguments_LINK_FLAGS} ... YES${_check_cached_status}" )
		endif()
		set_property(TARGET ${_parsedArguments_TARGET} APPEND_STRING PROPERTY LINK_FLAGS " ${_parsedArguments_LINK_FLAGS}")
	else()
		if(NOT _parsedArguments_QUIET)
			message( STATUS "Set TARGET ${_parsedArguments_TARGET} LINK_FLAG: ${_parsedArguments_LINK_FLAGS} ... no${_check_cached_status}" )
		endif()
	endif()

	if(NOT _parsedArguments_CACHED_RESULT_NAME)
		# Unset the temporary cached variable that CHECK_CXX_LINKER_FLAG populated
		unset(_cached_result_variable_name CACHE)
	endif()
endfunction()

INCLUDE(CheckCSourceRuns)
INCLUDE(CheckCSourceCompiles)
function(CHECK_C_LINKER_FLAGS _FLAGS _RESULT)
	set(CMAKE_REQUIRED_FLAGS "${_FLAGS}")
	set(CMAKE_REQUIRED_QUIET ON)
	set(_test_source "int main() { return 0; }")
	if(NOT CMAKE_CROSSCOMPILING)
		CHECK_C_SOURCE_RUNS("${_test_source}" ${_RESULT})
	else()
		CHECK_C_SOURCE_COMPILES("${_test_source}" ${_RESULT})
	endif()
	unset(CMAKE_REQUIRED_QUIET)
	set(CMAKE_REQUIRED_FLAGS "")
endfunction(CHECK_C_LINKER_FLAGS)

INCLUDE(CheckCXXSourceRuns)
INCLUDE(CheckCXXSourceCompiles)
function(CHECK_CXX_LINKER_FLAGS _FLAGS _RESULT)
	set(CMAKE_REQUIRED_FLAGS "${_FLAGS}")
	set(CMAKE_REQUIRED_QUIET ON)
	set(_test_source "int main() { return 0; }")
	if(NOT CMAKE_CROSSCOMPILING)
		CHECK_CXX_SOURCE_RUNS("${_test_source}" ${_RESULT})
	else()
		CHECK_CXX_SOURCE_COMPILES("${_test_source}" ${_RESULT})
	endif()
	unset(CMAKE_REQUIRED_QUIET)
	set(CMAKE_REQUIRED_FLAGS "")
endfunction(CHECK_CXX_LINKER_FLAGS)
