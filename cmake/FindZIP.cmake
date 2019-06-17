# - Try to find a supported ZIP compression executable
# Once done this will define
#
#  ZIP_FOUND - system has a zip executable
#  ZIP_EXECUTABLE - the zip executable
#
# This script supports detection of the following programs:
#	7-Zip (`7z`, `7za`)
#	`zip` (installed locally or through Cygwin)
#
# It also provides a function COMPRESS_ZIP that can be used to compress a list of files / folders,
# specifying various options. COMPRESS_ZIP supports all of the above detected ZIP_EXECUTABLE possibilities.
#
#
# Copyright © 2018-2020 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2020-06-03b
#

cmake_minimum_required(VERSION 3.3)

set(_PF32BIT "ProgramFiles(x86)")

# Search for a supported executable to use for ZIP compression
#
# The search is a modified version of the search code in: https://github.com/Kitware/CMake/blob/master/Modules/CPackZIP.cmake
# Which is distributed under the OSI-approved BSD 3-Clause License (https://cmake.org/licensing)
#

# Search for 7-Zip
find_program(ZIP_EXECUTABLE NAMES 7z 7za PATHS "$ENV{ProgramFiles}/7-Zip" "$ENV{${_PF32BIT}}/7-Zip" "$ENV{ProgramW6432}/7-Zip")
if(ZIP_EXECUTABLE MATCHES "7z|7za")
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


# COMPRESS_ZIP(OUTPUT outputFile
#			   [COMPRESSION_LEVEL <0 | 1 | 3 | 5 | 7 | 9>]
#			   PATHS [files...] [WORKING_DIRECTORY dir]
#			   [PATHS [files...] [WORKING_DIRECTORY dir]]
#			   [DEPENDS [depends...]]
#			   [IGNORE_GIT]
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
# Each set of PATHS may also optionally specify an associated WORKING_DIRECTORY.
#
# DEPENDS may be used to specify additional dependencies, which are appended to the
# auto-generated list of dependencies used for the internal call to `add_custom_command`.
#
# QUIET attempts to suppress (most) output from the ZIP_EXECUTABLE that is used.
# (This option may have no effect, if unsupported by the ZIP_EXECUTABLE.)
#
function(COMPRESS_ZIP)

	if(NOT ZIP_EXECUTABLE)
		message ( FATAL_ERROR "Unable to find zip executable. Unable to zip." )
	endif()

	set(_options ALL IGNORE_GIT QUIET)
	set(_oneValueArgs OUTPUT COMPRESSION_LEVEL) #WORKING_DIRECTORY)
	set(_multiValueArgs DEPENDS)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	# Check that OUTPUT was provided
	if(NOT _parsedArguments_OUTPUT)
		message( FATAL_ERROR "Missing required OUTPUT parameter" )
	endif()

	# Check compression level
	# Limited to certain compression levels because 7-zip only supports those
	if(NOT "${_parsedArguments_COMPRESSION_LEVEL}" MATCHES "^(0|1|3|5|7|9)$")
		message( FATAL_ERROR "Unsupported compression level \"${_parsedArguments_COMPRESSION_LEVEL}\" (must be: 0, 1, 3, 5, 7, 9)" )
	endif()

	if(ZIP_EXECUTABLE MATCHES "7z|7za")
		set(_zipExecutableOptions a -tzip -mtc=off)
		if(DEFINED _parsedArguments_COMPRESSION_LEVEL)
			# 7z command-line option for compression level (when in ZIP mode) is: "-mx=#"
			# only supports compression levels: 0, 1, 3, 5, 7, 9
			list(APPEND _zipExecutableOptions "-mx=${_parsedArguments_COMPRESSION_LEVEL}")
		endif()
		if(_parsedArguments_QUIET)
			if(7Z_SUPPORTS_SWITCH_BB0)
				list(APPEND _zipExecutableOptions "-bb0")
			endif()
		endif()
		if(_parsedArguments_IGNORE_GIT)
			list(APPEND _zipExecutableOptions "-xr!.git*")
		endif()
	elseif(ZIP_EXECUTABLE MATCHES "zip")
		set(_zipExecutableOptions -X -r)
		if(DEFINED _parsedArguments_COMPRESSION_LEVEL)
			# zip command-line option for compression level is "-#" (ex. "-0")
			list(APPEND _zipExecutableOptions "-${_parsedArguments_COMPRESSION_LEVEL}")
		endif()
		if(_parsedArguments_QUIET)
			list(APPEND _zipExecutableOptions "-q")
		endif()
		if(_parsedArguments_IGNORE_GIT)
			list(APPEND _zipExecutableOptions "--exclude='*/.git*'")
		endif()
	else()
		# COMPRESS_ZIP does not (yet) have support for the detected zip executable
		message( FATAL_ERROR "COMPRESS_ZIP does not currently support ZIP_EXECUTABLE: " ${ZIP_EXECUTABLE} )
	endif()

	# Check arguments "unparsed" by CMAKE_PARSE_ARGUMENTS for PATHS sets
	set(_COMMAND_LIST)
	set(_depends_PATHS)
	set(_inPATHSet FALSE)
	set(_expecting_WORKINGDIR FALSE)
	unset(_currentPATHSet_PATHS)
	unset(_currentPATHSet_WORKINGDIR)
	foreach(currentArg ${_parsedArguments_UNPARSED_ARGUMENTS})
		if("${currentArg}" STREQUAL "PATHS")
			if(_expecting_WORKINGDIR)
				# Provided "WORKING_DIRECTORY" keyword, but no variable after it
				message( FATAL_ERROR "WORKING_DIRECTORY keyword provided, but missing variable afterwards" )
			endif()
			if(_inPATHSet AND DEFINED _currentPATHSet_PATHS)
				# Ending one non-empty PATH set, beginning another
				if(NOT DEFINED _currentPATHSet_WORKINGDIR)
					set(_currentPATHSet_WORKINGDIR "${CMAKE_CURRENT_SOURCE_DIR}")
				endif()
				list(APPEND _COMMAND_LIST
					COMMAND
					${CMAKE_COMMAND} -E chdir ${_currentPATHSet_WORKINGDIR}
					${CMAKE_COMMAND} -E env LC_ALL=C LC_COLLATE=C
					${ZIP_EXECUTABLE} ${_zipExecutableOptions} ${_parsedArguments_OUTPUT} ${_currentPATHSet_PATHS}
				)
				foreach (_path ${_currentPATHSet_PATHS})
					set(_dependPath "${_currentPATHSet_WORKINGDIR}/${_path}")
					list(APPEND _depends_PATHS "${_dependPath}")
				endforeach()
			endif()
			set(_inPATHSet TRUE)
			unset(_currentPATHSet_PATHS)
			unset(_currentPATHSet_WORKINGDIR)
		elseif("${currentArg}" STREQUAL "WORKING_DIRECTORY")
			if(NOT _inPATHSet)
				message( FATAL_ERROR "WORKING_DIRECTORY must be specified at end of PATHS set" )
			endif()
			if(_expecting_WORKINGDIR)
				message( FATAL_ERROR "Duplicate WORKING_DIRECTORY keyword" )
			endif()
			if(DEFINED _currentPATHSet_WORKINGDIR)
				message( FATAL_ERROR "PATHS set has more than one WORKING_DIRECTORY keyword" )
			endif()
			set(_expecting_WORKINGDIR TRUE)
		elseif(_expecting_WORKINGDIR)
			set(_currentPATHSet_WORKINGDIR "${currentArg}")
			set(_expecting_WORKINGDIR FALSE)
		elseif(_inPATHSet)
			# Treat argument as a PATH
			list(APPEND _currentPATHSet_PATHS "${currentArg}")
		else()
			# Unexpected argument
			message( FATAL_ERROR "Unexpected argument: ${currentArg}" )
		endif()
	endforeach()
	if(_expecting_WORKINGDIR)
		# Provided "WORKING_DIRECTORY" keyword, but no variable after it
		message( FATAL_ERROR "WORKING_DIRECTORY keyword provided, but missing variable afterwards" )
	endif()
	if(_inPATHSet AND DEFINED _currentPATHSet_PATHS)
		# Ending one non-empty PATH set
		if(NOT DEFINED _currentPATHSet_WORKINGDIR)
			set(_currentPATHSet_WORKINGDIR "${CMAKE_CURRENT_SOURCE_DIR}")
		endif()
		list(APPEND _COMMAND_LIST
			COMMAND
			${CMAKE_COMMAND} -E chdir ${_currentPATHSet_WORKINGDIR}
			${ZIP_EXECUTABLE} ${_zipExecutableOptions} ${_parsedArguments_OUTPUT} ${_currentPATHSet_PATHS}
		)
		foreach (_path ${_currentPATHSet_PATHS})
			set(_dependPath "${_currentPATHSet_WORKINGDIR}/${_path}")
			list(APPEND _depends_PATHS "${_dependPath}")
		endforeach()
	endif()

	if(_parsedArguments_DEPENDS)
		list(APPEND _depends_PATHS ${_parsedArguments_DEPENDS})
	endif()

	add_custom_command(
		OUTPUT "${_parsedArguments_OUTPUT}"
		${_COMMAND_LIST}
		DEPENDS ${_depends_PATHS}
		WORKING_DIRECTORY "${_workingDirectory}"
		VERBATIM
	)

endfunction()
