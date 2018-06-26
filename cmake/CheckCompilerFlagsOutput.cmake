#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2018-07-08a
#

cmake_minimum_required(VERSION 3.5)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# CHECK_COMPILER_FLAGS_OUTPUT(	<compiler_flags>
#								COMPILER_TYPE <C | CXX>
#								[OUTPUT_FLAGS <output_compiler_flags> = compiler_flags]
#								OUTPUT_VARIABLE <output_variable>
#								[APPEND]
#								[FORCE]
#								[QUIET <ALL | FAILURES | OFF> = FAILURES] )
#
# CHECK_COMPILER_FLAGS_OUTPUT sets the OUTPUT_VARIABLE to the output flags if the current
# compiler seems to support the specified compiler flags.
#
# If COMPILER_TYPE C is specified, this checks if the configured C compiler seems
# to support the specified compiler flags. If COMPILER_TYPE CXX is specified, this
# checks if the configured CXX compiler seems to support the compiler flags.
#
# If OUTPUT_FLAGS is specified, and the compiler supports the input compiler_flags, the
# value of OUTPUT_FLAGS will be used to set / append to the OUTPUT_VARIABLE (instead
# of the input compiler_flags).
#
# If APPEND is specified, the output flags will be appended to the current contents of
# the OUTPUT_VARIABLE.
#
# If FORCE is specified, the value of this check will not be cached (and any
# previously-cached value will be ignored).
#
# QUIET can be specified to control message output with one of the following settings:
#	"ALL" - do NOT output any messages
#	"FAILIRES" - do NOT output messages on any failures
#	"OFF" - all output is enabled
#
function(CHECK_COMPILER_FLAGS_OUTPUT _compiler_flags)
	set(_options APPEND FORCE)
	set(_oneValueArgs COMPILER_TYPE OUTPUT_FLAGS OUTPUT_VARIABLE QUIET)
	set(_multiValueArgs)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _compiler_flags)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT requires compiler flags (first parameter)" )
	endif()

	if(DEFINED _parsedArguments_QUIET)
		if(NOT _parsedArguments_QUIET MATCHES "^(ALL|FAILURES|OFF)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT invalid QUIET value - must be one of: (ALL, FAILURES, OFF)" )
		endif()
	else()
		set(_parsedArguments_QUIET "FAILURES")
	endif()

	if(NOT DEFINED _parsedArguments_COMPILER_TYPE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT missing required COMPILER_TYPE value - must be one of: (C, CXX)" )
	else()
		if(NOT _parsedArguments_COMPILER_TYPE MATCHES "^(C|CXX)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
		endif()
	endif()

	if(NOT DEFINED _parsedArguments_OUTPUT_VARIABLE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT missing required OUTPUT_VARIABLE" )
	endif()

	if(DEFINED _parsedArguments_OUTPUT_FLAGS)
		set(_compiler_flags_output "${_parsedArguments_OUTPUT_FLAGS}")
	else()
		set(_compiler_flags_output "${_compiler_flags}")
	endif()

	set(_tmp_check_result)
	CHECK_COMPILER_FLAGS("${_compiler_flags}" COMPILER_TYPE "${_parsedArguments_COMPILER_TYPE}" RESULT_VARIABLE _tmp_check_result QUIET ALL)

	set(_compiler_type "${_parsedArguments_COMPILER_TYPE}")
	if(_tmp_check_result)
		if(NOT _parsedArguments_QUIET OR _parsedArguments_QUIET MATCHES "FAILURES")
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags_output} ... YES${_check_cached_status}" )
		endif()
		if(_parsedArguments_APPEND)
			set(${_parsedArguments_OUTPUT_VARIABLE} "${${_parsedArguments_OUTPUT_VARIABLE}} ${_compiler_flags_output}" PARENT_SCOPE)
		else()
			set(${_parsedArguments_OUTPUT_VARIABLE} "${_compiler_flags_output}" PARENT_SCOPE)
		endif()
	else()
		if(NOT _parsedArguments_QUIET)
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags_output} ... no${_check_cached_status}" )
		endif()
	endif()
endfunction()

function(CHECK_COMPILER_FLAGS _compiler_flags)
	set(_options FORCE)
	set(_oneValueArgs COMPILER_TYPE RESULT_VARIABLE QUIET)
	set(_multiValueArgs )

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _compiler_flags)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS requires compiler flags (first parameter)" )
	endif()

	if(DEFINED _parsedArguments_QUIET)
		if(NOT _parsedArguments_QUIET MATCHES "^(ALL|FAILURES|OFF)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid QUIET value - must be one of: (ALL, FAILURES, OFF)" )
		endif()
	else()
		set(_parsedArguments_QUIET "FAILURES")
	endif()

	if(NOT DEFINED _parsedArguments_COMPILER_TYPE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS missing required COMPILER_TYPE value - must be one of: (C, CXX)" )
	else()
		if(NOT _parsedArguments_COMPILER_TYPE MATCHES "^(C|CXX)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
		endif()
	endif()

	if(NOT DEFINED _parsedArguments_RESULT_VARIABLE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS missing required RESULT_VARIABLE" )
	endif()

	# Create a sanitized version of the input compiler flags string that can be used in a variable name
	string(REGEX REPLACE "[^a-zA-Z0-9\_]" "_" _cached_result_variable_flag_desc "${_compiler_flags}")

	set(_compiler_type "${_parsedArguments_COMPILER_TYPE}")
	if (NOT _parsedArguments_FORCE)
		set(_cached_result_variable_name "${_compiler_type}_COMPILER_FLAG_SUPPORTED_${_cached_result_variable_flag_desc}")
		if(DEFINED ${_cached_result_variable_name})
			set(_check_cached_status " (cached)")
		endif()
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

	if(_compiler_type MATCHES "CXX")
		CHECK_CXX_COMPILER_FLAG("${_compiler_flags}" ${_cached_result_variable_name})
	elseif(_compiler_type MATCHES "C")
		CHECK_C_COMPILER_FLAG("${_compiler_flags}" ${_cached_result_variable_name})
	else()
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
	endif()

	if(${_cached_result_variable_name})
		if(NOT _parsedArguments_QUIET OR _parsedArguments_QUIET MATCHES "FAILURES")
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags} ... YES${_check_cached_status}" )
		endif()
		set(${_parsedArguments_RESULT_VARIABLE} ON PARENT_SCOPE)
	else()
		if(NOT _parsedArguments_QUIET)
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags} ... no${_check_cached_status}" )
		endif()
		set(${_parsedArguments_RESULT_VARIABLE} OFF PARENT_SCOPE)
	endif()

	if (_parsedArguments_FORCE)
		# Unset the temporary cached variable that CHECK_<LANG>_COMPILER_FLAG populated
		unset(_cached_result_variable_name CACHE)
	endif()
endfunction()
