cmake_minimum_required(VERSION 3.5...3.14)
project (quickjs C)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

##################
# NOTES:
# This is a WZ-specific QuickJS CMake build config

get_filename_component(sourceDirRelFilePath "${CMAKE_CURRENT_SOURCE_DIR}/../../"
                       REALPATH)

set(CMAKE_MODULE_PATH "${sourceDirRelFilePath}/cmake")

##################
# QuickJS library

set(QUICKJS_HEADERS
	cutils.h
#	libbf.h
	libregexp.h
	libunicode.h
	list.h
	quickjs.h
	# extensions
	quickjs-debugger.h
	quickjs-limitedcontext.h
)

set(QUICKJS_SOURCES
	cutils.c
#	libbf.c
	libregexp.c
	libunicode.c
	quickjs.c
	# extensions
	# INCLUDED DIRECTLY IN QUICKJS.C: quickjs-debugger.c
	# INCLUDED DIRECTLY IN QUICKJS.C: quickjs-limitedcontext.c
)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" QUICKJS_VERSION_STR)

INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES("sys/time.h" HAVE_SYS_TIME_H LANGUAGE C)

add_library(quickjs STATIC ${QUICKJS_HEADERS} ${QUICKJS_SOURCES})
target_include_directories(quickjs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(quickjs PRIVATE
	CONFIG_VERSION="${QUICKJS_VERSION_STR}"
	_GNU_SOURCE
	QUICKJS_DISABLE_ATOMICS
)
if(HAVE_SYS_TIME_H)
	target_compile_definitions(quickjs PRIVATE QUICKJS_HAVE_SYS_TIME_H)
endif()
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(quickjs PRIVATE Threads::Threads)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	# GCC 9.3.0+ has been tested to be fine at -O2
	if((CMAKE_C_COMPILER_VERSION VERSION_LESS 9.3) AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 7.0))
		# Avoid SEGFAULT with earlier GCC and fcode-hoisting
		message(STATUS "QuickJS: Using -fno-code-hoisting (${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
		target_compile_options(quickjs PRIVATE -fno-code-hoisting)
	endif()
endif()

include(CheckCompilerFlagsOutput)
if(MSVC)
	# Disable some warnings for QuickJS
	set(QUICKJS_COMPILE_OPTIONS "")

	# warning C4018: '<': signed/unsigned mismatch
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4018)

	# warning C4146: unary minus operator applied to unsigned type, result still unsigned
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4146)

	# warning C4191: 'type cast': unsafe conversion from 'JSCFunctionMagic (__cdecl *)' to 'JSCFunction (__cdecl *)'
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4191)

	# warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4244)

	# warning C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4267)

	# warning C4334: 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
	set(QUICKJS_COMPILE_OPTIONS ${QUICKJS_COMPILE_OPTIONS} /wd4334)

	target_compile_options(quickjs PRIVATE ${QUICKJS_COMPILE_OPTIONS})
	target_compile_definitions(quickjs PRIVATE "_CRT_SECURE_NO_WARNINGS")
else()
	# Disable some warnings from QuickJS

	set(_supported_quickjs_c_compiler_flags "")

	# -Wcast-align				(GCC 3.4+, Clang 3.2+)
	check_compiler_flags_output("-Werror -Wno-cast-align -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-cast-align" OUTPUT_VARIABLE _supported_quickjs_c_compiler_flags APPEND)

	# -Wshadow					(GCC 3.4+, Clang 3.2+)
	check_compiler_flags_output("-Werror -Wno-shadow -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-shadow" OUTPUT_VARIABLE _supported_quickjs_c_compiler_flags APPEND)

	# -Wimplicit-int-float-conversion			(Clang)
	check_compiler_flags_output("-Werror -Wno-implicit-int-float-conversion -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-implicit-int-float-conversion" OUTPUT_VARIABLE _supported_quickjs_c_compiler_flags APPEND)

	# -Wimplicit-const-int-float-conversion		(Clang)
	check_compiler_flags_output("-Werror -Wno-implicit-const-int-float-conversion -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-implicit-const-int-float-conversion" OUTPUT_VARIABLE _supported_quickjs_c_compiler_flags APPEND)

	if (NOT _supported_quickjs_c_compiler_flags STREQUAL "")
		string(REPLACE " " ";" _supported_quickjs_c_compiler_flags "${_supported_quickjs_c_compiler_flags}")
		target_compile_options(quickjs PRIVATE ${_supported_quickjs_c_compiler_flags})
	endif()
endif()
