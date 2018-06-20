# - Try to find a supported ZIP compression executable
# Once done this will define
#
#  ZIP_FOUND - system has a zip executable
#  ZIP_EXECUTABLE - the zip executable
#
# This script supports detection of the following programs:
#	7-Zip (`7z`)
#	`zip` (installed locally or through Cygwin)
#
# It also provides a function COMPRESS_ZIP that can be used to compress a list of files / folders,
# specifying various options. COMPRESS_ZIP supports all of the above detected ZIP_EXECUTABLE possibilities.
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2018-02-18a
#

cmake_minimum_required(VERSION 3.3)

set(_PF32BIT "ProgramFiles(x86)")

# Search for a supported executable to use for ZIP compression
#
# The search is a modified version of the search code in: https://github.com/Kitware/CMake/blob/master/Modules/CPackZIP.cmake
# Which is distributed under the OSI-approved BSD 3-Clause License (https://cmake.org/licensing)
#

# Search for 7-Zip
find_program(ZIP_EXECUTABLE 7z PATHS "$ENV{ProgramFiles}/7-Zip" "$ENV{${_PF32BIT}}/7-Zip" "$ENV{ProgramW6432}/7-Zip")
if(ZIP_EXECUTABLE MATCHES "7z")
	# Test whether 7-Zip supports the "-bb0" option to disable log output
	execute_process(COMMAND ${ZIP_EXECUTABLE} i -bb0
					RESULT_VARIABLE 7z_bb_result
					OUTPUT_VARIABLE 7z_bb_info
					OUTPUT_STRIP_TRAILING_WHITESPACE
					ERROR_QUIET
	)
	if (7z_bb_result EQUAL 0)
		message( STATUS "7z supports switch: -bb0 ... YES" )
		set(7Z_SUPPORTS_SWITCH_BB0 ON CACHE BOOL "7z supports switch: -bb0")
	else()
		message( STATUS "7z supports switch: -bb0 ... no" )
		set(7Z_SUPPORTS_SWITCH_BB0 OFF CACHE BOOL "7z supports switch: -bb0")
	endif()
	MARK_AS_ADVANCED(7Z_SUPPORTS_SWITCH_BB0)
endif()

if(NOT ZIP_EXECUTABLE)
	# Search for "zip"
	find_package(Cygwin)
	find_program(ZIP_EXECUTABLE zip PATHS "${CYGWIN_INSTALL_PATH}/bin")
endif()


# Support find_package(ZIP [REQUIRED])
#
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZIP REQUIRED_VARS ZIP_EXECUTABLE)
MARK_AS_ADVANCED(ZIP_EXECUTABLE)


# COMPRESS_ZIP(outputFile
#			   [COMPRESSION_LEVEL <0 | 1 | 3 | 5 | 7 | 9>]
#			   [WORKING_DIRECTORY dir]
#			   PATHS file1 ...  fileN
#			   [QUIET])
#
# Compress a list of files / folders into a ZIP file, named <outputFile>.
# Any directories specified will cause the directory's contents to be recursively included.
#
# If COMPRESSION_LEVEL is specified, the ZIP compression level setting will be passed
# through to the ZIP_EXECUTABLE. A compression level of "0" means no compression.
#
# If WORKING_DIRECTORY is specified, the WORKING_DIRECTORY will be set for the execution of
# the ZIP_EXECUTABLE.
#
function(COMPRESS_ZIP _outputFile)

	if(NOT ZIP_EXECUTABLE)
		message ( FATAL_ERROR "Unable to find zip executable. Unable to zip." )
	endif()

	set(_options ALL QUIET)
	set(_oneValueArgs COMPRESSION_LEVEL WORKING_DIRECTORY)
	set(_multiValueArgs PATHS)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	# Check compression level
	# Limited to certain compression levels because 7-zip only supports those
	if(NOT "${_parsedArguments_COMPRESSION_LEVEL}" MATCHES "^(0|1|3|5|7|9)$")
		message( FATAL_ERROR "Unsupported compression level \"${_parsedArguments_COMPRESSION_LEVEL}\" (must be: 0, 1, 3, 5, 7, 9)" )
	endif()

	if(_parsedArguments_WORKING_DIRECTORY)
		set(_workingDirectory ${_parsedArguments_WORKING_DIRECTORY})
	else()
		set(_workingDirectory ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# Build depends paths
	set(_depends_PATHS)
	foreach (_path ${_parsedArguments_PATHS})
		set(_dependPath "${_workingDirectory}/${_path}")
		list(APPEND _depends_PATHS "${_dependPath}")
	endforeach()

	if(ZIP_EXECUTABLE MATCHES "7z")
		set(_additionalOptions)
		if(DEFINED _parsedArguments_COMPRESSION_LEVEL)
			# 7z command-line option for compression level (when in ZIP mode) is: "-mx=#"
			# only supports compression levels: 0, 1, 3, 5, 7, 9
			list(APPEND _additionalOptions "-mx=${_parsedArguments_COMPRESSION_LEVEL}")
		endif()
		if(_parsedArguments_QUIET)
			if(7Z_SUPPORTS_SWITCH_BB0)
				list(APPEND _additionalOptions "-bb0")
			endif()
		endif()

		add_custom_command(
			OUTPUT "${_outputFile}"
			COMMAND ${ZIP_EXECUTABLE} a -tzip ${_additionalOptions} ${_outputFile} ${_parsedArguments_PATHS}
			DEPENDS ${_depends_PATHS}
			WORKING_DIRECTORY "${_workingDirectory}"
			VERBATIM
		)
	elseif(ZIP_EXECUTABLE MATCHES "zip")
		set(_additionalOptions)
		if(DEFINED _parsedArguments_COMPRESSION_LEVEL)
			# zip command-line option for compression level is "-#" (ex. "-0")
			list(APPEND _additionalOptions "-${_parsedArguments_COMPRESSION_LEVEL}")
		endif()
		if(_parsedArguments_QUIET)
			list(APPEND _additionalOptions "-q")
		endif()

		add_custom_command(
			OUTPUT "${_outputFile}"
			COMMAND ${ZIP_EXECUTABLE} -r ${_additionalOptions} ${_outputFile} ${_parsedArguments_PATHS}
			DEPENDS ${_depends_PATHS}
			WORKING_DIRECTORY "${_workingDirectory}"
			VERBATIM
		)
	else()
		# COMPRESS_ZIP does not (yet) have support for the detected zip executable
		message( FATAL_ERROR "COMPRESS_ZIP does not currently support ZIP_EXECUTABLE: " ${ZIP_EXECUTABLE} )
	endif()

endfunction()
