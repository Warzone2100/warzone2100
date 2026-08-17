# Regenerates a build manifest for an executable shipped beside another build's data archives
# (ex. the Windows installer, which pairs every architecture's executable with the x64 data archives).
# The publisher, version, commit, origin, platform and attribution are taken from the source manifest.
#
# Usage: cmake -DWZBM_...=... -P buildmanifest_rebase.cmake
#
# Required:
#   WZBM_SOURCE_MANIFEST - the executable's own manifest (signed or unsigned)
#   WZBM_EXE             - path to the executable
#   WZBM_DATA_DIR        - directory containing the data archives it ships with
#   WZBM_OUTPUT          - output manifest path (may be WZBM_SOURCE_MANIFEST)
# Optional:
#   WZBM_TOOL, WZBM_KEY_FILE, WZBM_KEY_ENV, WZBM_KEY_ID - as for buildmanifest_generate.cmake

cmake_minimum_required(VERSION 3.19)

foreach(_var WZBM_SOURCE_MANIFEST WZBM_EXE WZBM_DATA_DIR WZBM_OUTPUT)
	if(NOT DEFINED ${_var})
		message(FATAL_ERROR "Missing required variable: ${_var}")
	endif()
endforeach()
# Windows callers pass native paths, which file(GLOB) does not accept
foreach(_var WZBM_SOURCE_MANIFEST WZBM_EXE WZBM_DATA_DIR WZBM_OUTPUT WZBM_TOOL WZBM_KEY_FILE)
	if(DEFINED ${_var})
		file(TO_CMAKE_PATH "${${_var}}" ${_var})
	endif()
endforeach()

file(READ "${WZBM_SOURCE_MANIFEST}" _source)
string(REPLACE "\r\n" "\n" _source "${_source}")

# A signed envelope carries the manifest verbatim between its begin and signature markers
set(_begin_marker "-----BEGIN WZ2100 BUILD MANIFEST-----\n")
string(FIND "${_source}" "${_begin_marker}" _begin)
if(_begin EQUAL 0)
	string(LENGTH "${_begin_marker}" _begin_length)
	string(FIND "${_source}" "\n-----WZ2100 SIGNATURE-----" _signature)
	if(_signature EQUAL -1)
		message(FATAL_ERROR "Malformed signed manifest: ${WZBM_SOURCE_MANIFEST}")
	endif()
	math(EXPR _payload_length "${_signature} - ${_begin_length}")
	string(SUBSTRING "${_source}" ${_begin_length} ${_payload_length} _source)
endif()

string(JSON _publisher GET "${_source}" publisher)
string(JSON _version GET "${_source}" version)
string(JSON _commit GET "${_source}" commit)
string(JSON _origin ERROR_VARIABLE _no_origin GET "${_source}" origin)
string(JSON _platform GET "${_source}" platform)
string(JSON _notice GET "${_source}" attribution notice)
string(JSON _copyright GET "${_source}" attribution copyright)
string(JSON _contact ERROR_VARIABLE _no_contact GET "${_source}" attribution contact)

set(_generate_args
	"-DWZBM_EXE=${WZBM_EXE}"
	"-DWZBM_DATA_DIR=${WZBM_DATA_DIR}"
	"-DWZBM_OUTPUT=${WZBM_OUTPUT}"
	"-DWZBM_VERSION=${_version}"
	"-DWZBM_COMMIT=${_commit}"
	"-DWZBM_PLATFORM=${_platform}"
	"-DWZBM_PUBLISHER=${_publisher}"
	"-DWZBM_NOTICE=${_notice}"
)
if(_copyright MATCHES "2005-([0-9]+) Warzone 2100 Project")
	list(APPEND _generate_args "-DWZBM_COPYRIGHT_YEAR=${CMAKE_MATCH_1}")
endif()
if(NOT _no_origin)
	list(APPEND _generate_args "-DWZBM_ORIGIN=${_origin}")
endif()
if(NOT _no_contact)
	list(APPEND _generate_args "-DWZBM_CONTACT=${_contact}")
endif()
foreach(_var WZBM_TOOL WZBM_KEY_FILE WZBM_KEY_ENV WZBM_KEY_ID)
	if(DEFINED ${_var})
		list(APPEND _generate_args "-D${_var}=${${_var}}")
	endif()
endforeach()

execute_process(
	COMMAND ${CMAKE_COMMAND} ${_generate_args} -P "${CMAKE_CURRENT_LIST_DIR}/buildmanifest_generate.cmake"
	RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
	message(FATAL_ERROR "Build manifest regeneration failed for: ${WZBM_EXE}")
endif()
