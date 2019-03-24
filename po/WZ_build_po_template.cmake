cmake_minimum_required(VERSION 3.5)

# Build the warzone2100.pot default language file
# from the files listed in POTFILES.in
#
# Required input defines:
# - POTFILES_IN: the path to POTFILES.in
# - OUTPUT_FILE: the path where the output file (ex. warzone2100.pot) should be written
#
# Optional input defines:
# - XGETTEXT_CMD: the command used to execute xgettext
# - TEMP_DIR: a directory in which temporary files should be written (if not the same directory as the OUTPUT_FILE)
#

if(NOT DEFINED POTFILES_IN OR "${POTFILES_IN}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: POTFILES_IN" )
endif()

if(NOT DEFINED OUTPUT_FILE OR "${OUTPUT_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: OUTPUT_FILE" )
endif()

if(DEFINED TEMP_DIR)
	if(NOT EXISTS "${TEMP_DIR}" OR NOT IS_DIRECTORY "${TEMP_DIR}")
		message( FATAL_ERROR "TEMP_DIR does not exist, or is not a directory: \"${TEMP_DIR}\"" )
	endif()
endif()

#################################

if(NOT DEFINED XGETTEXT_CMD)
	find_program(XGETTEXT_CMD xgettext)

	if(NOT XGETTEXT_CMD)
		MESSAGE( FATAL_ERROR "xgettext not found. Unable to generate Language translations without Gettext." )
		return()
	endif()

	MARK_AS_ADVANCED(XGETTEXT_CMD)
else()
	message( STATUS "Using provided XGETTEXT_CMD: \"${XGETTEXT_CMD}\"" )
endif()

# Generate a temporary output file suffix that ought to be unique
if(NOT DEFINED TEMP_DIR)
	set(_tmp_outputfile_prefix "${OUTPUT_FILE}")
else()
	set(_tmp_outputfile_prefix "${TEMP_DIR}/wz2100.pot.tmpdir")
endif()
string(TIMESTAMP _output_timestamp "%Y%m%d%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET "0123456789abcdefghijklmnopqrstuvwxyz" _output_random)
set(_counter 0)
while(EXISTS "${_tmp_outputfile_prefix}.${_output_timestamp}-${_output_random}${_counter}.tmp")
	math(EXPR _counter "${_counter} + 1")
endwhile()
set(_xgettext_temp_output_file "${_tmp_outputfile_prefix}.${_output_timestamp}-${_output_random}${_counter}.tmp")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ _xgettext_temp_output_file: ${_xgettext_temp_output_file}")

# Configure the xgettext options
# NOTE: This matches what Makevars "XGETTEXT_OPTIONS" (and other values) provides
set(_xgettext_option_list --language=C++
	--from-code=UTF-8
	--no-wrap
	--width=1
	--keyword=_
	--keyword=N_
	--keyword=P_:1c,2
	--keyword=NP_:1c,2
	--package-name=warzone2100
	--package-version=${PROJECT_VERSION}
	--msgid-bugs-address=warzone2100-project@lists.sourceforge.net)

set(_xgettext_COPYRIGHT_HOLDER "Warzone 2100 Project")

# Build the .pot file
# Note: This uses the `--files-from=` parameter instead of passing in a giant command-line of files
#		because the latter option likes to break when building on Windows.
execute_process(
	COMMAND ${XGETTEXT_CMD} --add-comments=TRANSLATORS: ${_xgettext_option_list} --copyright-holder=${_xgettext_COPYRIGHT_HOLDER} -o ${_xgettext_temp_output_file} --files-from=${POTFILES_IN}
	RESULT_VARIABLE exstatus
)

if(NOT exstatus EQUAL 0)
	message( FATAL_ERROR "xgettext FAILED with error ${exstatus}: ${_xgettext_temp_output_file}" )
endif()

if(NOT EXISTS "${_xgettext_temp_output_file}")
	message( FATAL_ERROR "xgettext returned success, but temporary output file is not present!" )
endif()

if(EXISTS "${OUTPUT_FILE}")
	# Only copy the temporary output file over the OUTPUT_FILE if the new (temporary) output actually has different content
	# NOTE: This ignores certain header lines that update every time xgettext is run

	file(READ "${_xgettext_temp_output_file}" _strings_NEW_OUTPUT_FILE
		ENCODING UTF-8
	)
	string(REGEX REPLACE "\n\"POT-Creation-Date: [^\"]*\"\n" "\n" _strings_NEW_OUTPUT_FILE "${_strings_NEW_OUTPUT_FILE}")
	string(REGEX REPLACE "\n\"PO-Revision-Date: [^\"]*\"\n" "\n" _strings_NEW_OUTPUT_FILE "${_strings_NEW_OUTPUT_FILE}")

	file(READ "${OUTPUT_FILE}" _strings_OLD_OUTPUT_FILE
		ENCODING UTF-8
	)
	string(REGEX REPLACE "\n\"POT-Creation-Date: [^\"]*\"\n" "\n" _strings_OLD_OUTPUT_FILE "${_strings_OLD_OUTPUT_FILE}")
	string(REGEX REPLACE "\n\"PO-Revision-Date: [^\"]*\"\n" "\n" _strings_OLD_OUTPUT_FILE "${_strings_OLD_OUTPUT_FILE}")

	if (_strings_NEW_OUTPUT_FILE STREQUAL _strings_OLD_OUTPUT_FILE)
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ No pertinent changes: ${OUTPUT_FILE}")
		file(REMOVE "${_xgettext_temp_output_file}")
	else()
		# Contents changed - copy the new (temporary) output file to the desired output file location
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Changes detected - saving: ${OUTPUT_FILE}")
		file(RENAME "${_xgettext_temp_output_file}" "${OUTPUT_FILE}")
	endif()
else()
	# Since the OUTPUT_FILE doesn't exist, just rename the temporary output file
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Saving output file: ${OUTPUT_FILE}")
	file(RENAME "${_xgettext_temp_output_file}" "${OUTPUT_FILE}")
endif()
