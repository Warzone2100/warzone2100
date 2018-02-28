cmake_minimum_required(VERSION 3.5)

# A cross-platform script to support JS doc generation
#
# Requires:
#	- grep (both BSD and GNU grep have been tested)
#	or
#	- findstr (available on all modern Windows versions)
#
# To call this script, ensure that the working directory is set to the warzone source root,
# and define: OUTPUT_DIR
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

if(NOT DEFINED OUTPUT_DIR)
	message( FATAL_ERROR "OUTPUT_DIR must be defined" )
endif()

# PROCESS_LINES(<inputfile>
#				[MATCHING_LINE_REGEX <regex>]
#				[STRIP_LINE_PREFIX_REGEX <regex>]
#				OUTPUT_FILE <file>
#				[APPEND])
#
# IMPORTANT: Because MATCHING_LINE_REGEX may be passed to either grep or findstr (on Windows),
#			 the syntax used should be compatible with *all* of:
#				- BSD grep -e
#				- GNU grep -e
#				- findstr /r /c: (Windows)
#
#			 STRIP_LINE_PREFIX_REGEX is passed to CMake's string(REGEX REPLACE) and should be limited to
#			 what is supported by CMake's regex engine. Any text on a line after the STRIP_LINE_PREFIX_REGEX
#			 expression is preserved; any prior text on the line (and the match for STRIP_LINE_PREFIX_REGEX
#			 itself) is removed.
#
function(PROCESS_LINES inputfile)

	set(_options APPEND)
	set(_oneValueArgs MATCHING_LINE_REGEX STRIP_LINE_PREFIX_REGEX OUTPUT_FILE)
	set(_multiValueArgs)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _parsedArguments_OUTPUT_FILE)
		message( FATAL_ERROR "Missing required OUTPUT_FILE" )
	endif()

	if(NOT EXISTS "${inputfile}")
		message( FATAL_ERROR "Input file doesn't exist: \"${inputfile}\"" )
	endif()

	# Since the input path is being passed to native command-line tools, convert it to native path format
	file(TO_NATIVE_PATH "${inputfile}" inputfile)

	# CMake's handling of semicolons makes attempting to read in the file and do per-line processing
	# (while retaining semicolons) troublesome. Instead, use grep (or findstr on Windows) to do the
	# initial heavy-lifting.

	if(_parsedArguments_MATCHING_LINE_REGEX)
		find_program(GREP grep)
		if(GREP)
			# Use grep
			execute_process( COMMAND ${GREP} "${inputfile}" -e "${_parsedArguments_MATCHING_LINE_REGEX}"
				 OUTPUT_VARIABLE _matching_lines
			)
		else()
			# Windows likely doesn't have grep, but it *does* have "findstr" which should work for our purposes
			find_program(FINDSTR findstr)
			if(FINDSTR)
				# Use findstr
				execute_process( COMMAND ${FINDSTR} /r /c:${_parsedArguments_MATCHING_LINE_REGEX} "${inputfile}"
					 OUTPUT_VARIABLE _matching_lines
				)
			else()
				# No other fallback currently available
				message( FATAL_ERROR "Unable to process lines from files because grep (or an alternative) is not available." )
			endif()
		endif()
	else()
		# Just read in the file with file(READ)
		file(READ "${inputfile}" _matching_lines)
	endif()

	if(_parsedArguments_STRIP_LINE_PREFIX_REGEX)
		string(REGEX REPLACE "([^\n]*)(${_parsedArguments_STRIP_LINE_PREFIX_REGEX})([^\n]*(\n|$))" "\\3" _matching_lines "${_matching_lines}")
	endif()

	if(_parsedArguments_APPEND)
		file(APPEND "${_parsedArguments_OUTPUT_FILE}" "${_matching_lines}")
	else()
		file(WRITE "${_parsedArguments_OUTPUT_FILE}" "${_matching_lines}")
	endif()
endfunction()

PROCESS_LINES("src/qtscript.cpp"
				MATCHING_LINE_REGEX "//=="
				STRIP_LINE_PREFIX_REGEX "== ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-globals.md")

PROCESS_LINES("src/qtscriptfuncs.cpp"
				MATCHING_LINE_REGEX "//=="
				STRIP_LINE_PREFIX_REGEX "//== ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-globals.md" APPEND)

PROCESS_LINES("src/qtscript.cpp"
				MATCHING_LINE_REGEX "//__"
				STRIP_LINE_PREFIX_REGEX "//__ ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-events.md")

PROCESS_LINES("src/qtscript.cpp"
				MATCHING_LINE_REGEX "//--"
				STRIP_LINE_PREFIX_REGEX "//-- ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-functions.md")

PROCESS_LINES("src/qtscriptfuncs.cpp"
				MATCHING_LINE_REGEX "//--"
				STRIP_LINE_PREFIX_REGEX "//-- ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-functions.md" APPEND)

PROCESS_LINES("src/qtscript.cpp"
				MATCHING_LINE_REGEX "//[;][;]"
				STRIP_LINE_PREFIX_REGEX "//[;][;] ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-objects.md")

PROCESS_LINES("src/qtscriptfuncs.cpp"
				MATCHING_LINE_REGEX "//[;][;]"
				STRIP_LINE_PREFIX_REGEX "//[;][;] ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-objects.md" APPEND)

PROCESS_LINES("data/base/script/campaign/libcampaign.js"
				MATCHING_LINE_REGEX ".*//[;][;]"
				STRIP_LINE_PREFIX_REGEX "//[;][;] ?"
				OUTPUT_FILE "${OUTPUT_DIR}/js-campaign.md")

message( STATUS "Finished generating JS docs" )
