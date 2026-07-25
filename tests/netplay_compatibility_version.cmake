cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED PROJECT_SOURCE_DIR OR "${PROJECT_SOURCE_DIR}" STREQUAL "")
	message(FATAL_ERROR "PROJECT_SOURCE_DIR is required")
endif()
if(NOT DEFINED OUTPUT_DIRECTORY OR "${OUTPUT_DIRECTORY}" STREQUAL "")
	message(FATAL_ERROR "OUTPUT_DIRECTORY is required")
endif()

set(_template_file "${PROJECT_SOURCE_DIR}/lib/netplay/netplay_config.h.in")
set(_generator_file "${PROJECT_SOURCE_DIR}/lib/netplay/autorevision_netplay.cmake")
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")

function(GENERATE_CONFIG _result_var _name _tag _tag_count _branch _commit_count)
	set(_output_file "${OUTPUT_DIRECTORY}/${_name}/netplay_config.h")

	# Pass deliberately different release metadata to each invocation. The
	# generator must ignore it and must not require an autorevision cache.
	execute_process(
		COMMAND "${CMAKE_COMMAND}"
			-DVCS_TAG=${_tag}
			-DVCS_TAG_TAG_COUNT=${_tag_count}
			-DVCS_BRANCH=${_branch}
			-DVCS_COMMIT_COUNT=${_commit_count}
			-DTEMPLATE_FILE=${_template_file}
			-DOUTPUT_FILE=${_output_file}
			-P ${_generator_file}
		RESULT_VARIABLE _result
		OUTPUT_VARIABLE _output
		ERROR_VARIABLE _error
	)
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "Compatibility version generation failed:\n${_output}${_error}")
	endif()

	file(READ "${_output_file}" _generated_header)
	set(${_result_var} "${_generated_header}" PARENT_SCOPE)
endfunction()

GENERATE_CONFIG(_point_release_a "point-release-a" "4.5.1" 41 "release/4.5" 12000)
GENERATE_CONFIG(_point_release_b "point-release-b" "4.5.2" 42 "master" 12500)

if(NOT _point_release_a STREQUAL _point_release_b)
	message(FATAL_ERROR "Release metadata changed the multiplayer compatibility version")
endif()

string(REGEX MATCH "NETCODE_VERSION_MAJOR = (0x[0-9A-Fa-f]+|[0-9]+);" _major_match "${_point_release_a}")
if(NOT _major_match)
	message(FATAL_ERROR "Generated compatibility major version is not an integer")
endif()
set(_major "${CMAKE_MATCH_1}")

string(REGEX MATCH "NETCODE_VERSION_MINOR = (0x[0-9A-Fa-f]+|[0-9]+);" _minor_match "${_point_release_a}")
if(NOT _minor_match)
	message(FATAL_ERROR "Generated compatibility minor version is not an integer")
endif()
set(_minor "${CMAKE_MATCH_1}")

math(EXPR _major_value "${_major}")
math(EXPR _minor_value "${_minor}")
if(_major_value EQUAL 0 AND _minor_value EQUAL 0)
	message(FATAL_ERROR "Compatibility version 0.0 is reserved for lobby availability checks")
endif()
