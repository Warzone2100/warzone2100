cmake_minimum_required(VERSION 3.5)

# Required input defines:
# - INPUT_FILE: the path to the input file
# - OUTPUT_FILE: the full filename + path for the output file
#
# Optional input defines:
# - SKIP_IF_OUTPUT_EXISTS: ON or OFF (default)
# - SKIP_IF_INPUT_MISSING: ON or OFF (default)
#

if(NOT EXISTS "${INPUT_FILE}")
	if(NOT SKIP_IF_INPUT_MISSING)
		message( FATAL_ERROR "Input file does not exist at: \"${INPUT_FILE}\"" )
	endif()
	return()
endif()

if(SKIP_IF_OUTPUT_EXISTS AND EXISTS "${OUTPUT_FILE}")
	message( STATUS "Skipping copy; output file already exists at: \"${OUTPUT_FILE}\"" )
	return()
endif()

# Copy source file to destination path
configure_file("${INPUT_FILE}" "${OUTPUT_FILE}" COPYONLY)

