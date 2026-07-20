#
# Provides a function COMPRESS_ZIP that is compatible with the function in FindZIP.cmake, but just copies the files to a folder
#
#
# Copyright © 2018-2024 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2025-07-23a
#

cmake_minimum_required(VERSION 3.16...3.31)

set(_THIS_MODULE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# COMPRESS_ZIP(OUTPUT outputFile
#			   [COMPRESSION_LEVEL <0 | 1 | 3 | 5 | 7 | 9>]
#			   PATHS [files...] [WORKING_DIRECTORY dir]
#			   [PATHS [files...] [WORKING_DIRECTORY dir]]
#			   [DEPENDS [depends...]]
#			   [BUILD_ALWAYS_TARGET [target name]]
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
# If BUILD_ALWAYS_TARGET is specified, uses add_custom_target to create a target that is always built.
#
# QUIET attempts to suppress (most) output from the ZIP_EXECUTABLE that is used.
# (This option may have no effect, if unsupported by the ZIP_EXECUTABLE.)
#
function(COMPRESS_ZIP)

	set(_options ALL IGNORE_GIT QUIET)
	set(_oneValueArgs OUTPUT COMPRESSION_LEVEL BUILD_ALWAYS_TARGET) #WORKING_DIRECTORY)
	set(_multiValueArgs DEPENDS)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	# Check that OUTPUT was provided
	if(NOT _parsedArguments_OUTPUT)
		message( FATAL_ERROR "Missing required OUTPUT parameter" )
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
				foreach (_path ${_currentPATHSet_PATHS})
					list(APPEND _COMMAND_LIST
						COMMAND
						${CMAKE_COMMAND} -E chdir ${_currentPATHSet_WORKINGDIR}
						${CMAKE_COMMAND} -DSOURCE=${_path} -DDEST_DIR=${_parsedArguments_OUTPUT} -P ${_THIS_MODULE_BASE_DIR}/EmscriptenCompressZipCopy.cmake
					)
					set(_dependPath "${_currentPATHSet_WORKINGDIR}/${_path}")
#					list(APPEND _COMMAND_LIST
#						COMMAND
#						${CMAKE_COMMAND} -DSOURCE=${_dependPath} -DDEST_DIR=${_parsedArguments_OUTPUT} -P ${_THIS_MODULE_BASE_DIR}/EmscriptenCompressZipCopy.cmake
#					)
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
		foreach (_path ${_currentPATHSet_PATHS})
			set(_dependPath "${_currentPATHSet_WORKINGDIR}/${_path}")
			list(APPEND _COMMAND_LIST
				COMMAND
				${CMAKE_COMMAND} -E chdir ${_currentPATHSet_WORKINGDIR}
				${CMAKE_COMMAND} -DSOURCE=${_path} -DDEST_DIR=${_parsedArguments_OUTPUT} -P ${_THIS_MODULE_BASE_DIR}/EmscriptenCompressZipCopy.cmake
			)
			list(APPEND _depends_PATHS "${_dependPath}")
		endforeach()
	endif()

	if(_parsedArguments_DEPENDS)
		list(APPEND _depends_PATHS ${_parsedArguments_DEPENDS})
	endif()

	if(NOT _parsedArguments_BUILD_ALWAYS_TARGET)
		add_custom_command(
			OUTPUT "${_parsedArguments_OUTPUT}"
			${_COMMAND_LIST}
			DEPENDS ${_depends_PATHS}
			WORKING_DIRECTORY "${_workingDirectory}"
			VERBATIM
		)
	else()
		add_custom_target(
			${_parsedArguments_BUILD_ALWAYS_TARGET} ALL
			${_COMMAND_LIST}
			DEPENDS ${_depends_PATHS}
			WORKING_DIRECTORY "${_workingDirectory}"
			VERBATIM
		)
	endif()

endfunction()
