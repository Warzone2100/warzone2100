#
# Copyright © 2021 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2021-05-21a
#

INCLUDE(CheckCXXSourceRuns)
INCLUDE(CheckCXXSourceCompiles)

# CHECK_CXX_STD_THREAD( <_RESULT>
#						  [QUIET] )
#
# CHECK_CXX_STD_THREAD checks if std::thread seems to be supported by the current (specified) compiler + linker.
#
function(CHECK_CXX_STD_THREAD _RESULT)
	set(_options QUIET)
	set(_oneValueArgs)
	set(_multiValueArgs)
	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	set(_check_cached_status)
	if(DEFINED "${_RESULT}")
		set(_check_cached_status " (cached)")
	endif()

	find_package(Threads QUIET)
	if (TARGET Threads::Threads)
		set(CMAKE_REQUIRED_LIBRARIES "Threads::Threads")
	endif()
	if(_parsedArguments_QUIET)
		set(CMAKE_REQUIRED_QUIET OFF)
	endif()
	set(_test_source
		"#include<thread>\n \
		void foo() { /* do nothing */ }\n \
		int main() {\n \
			std::thread a (foo);\n \
			a.join();\n \
			return 0;\n \
		}"
	)
	if(NOT CMAKE_CROSSCOMPILING AND NOT CMAKE_GENERATOR_PLATFORM)
		CHECK_CXX_SOURCE_RUNS("${_test_source}" ${_RESULT})
	else()
		CHECK_CXX_SOURCE_COMPILES("${_test_source}" ${_RESULT})
	endif()
	if(_parsedArguments_QUIET)
		unset(CMAKE_REQUIRED_QUIET)
	endif()
	if(NOT _parsedArguments_QUIET)
		if(${_RESULT})
			message( STATUS "std::thread support... YES${_check_cached_status}" )
		else()
			message( STATUS "std::thread support... no${_check_cached_status}" )
		endif()
	endif()
	set(CMAKE_REQUIRED_LIBRARIES "")
endfunction(CHECK_CXX_STD_THREAD)
