# Generates the build manifest, signed when a key is provided.
#
# Usage: cmake -DWZBM_...=... -P buildmanifest_generate.cmake
#
# Required:
#   WZBM_EXE         - path to the final (code-signed) game executable
#   WZBM_OUTPUT      - output manifest path
#   WZBM_VERSION     - version string
#   WZBM_COMMIT      - full git commit hash
#   WZBM_PLATFORM    - platform artifact identifier (ex. windows-x64)
#   WZBM_DATA_DIR    - directory containing the core data archives (base.wz, mp.wz), unless WZBM_DATA_ARCHIVES is OFF
# Optional:
#   WZBM_TOOL        - path to the wzbuildmanifest executable (needed only to sign, or for the Mach-O canonical hash)
#   WZBM_PUBLISHER   - publisher id (default: self-built)
#   WZBM_ORIGIN      - where the source came from (ex. github.com/Warzone2100/warzone2100)
#   WZBM_CONTACT     - publisher contact
#   WZBM_NOTICE      - overrides the default attribution notice text
#   WZBM_COPYRIGHT_YEAR - latest copyright year (ex. the most recent commit's year), at least 2026
#   WZBM_KEY_FILE    - Ed25519 secret key file (unsigned if neither this nor WZBM_KEY_ENV is provided)
#   WZBM_KEY_ENV     - name of an environment variable holding the base64 Ed25519 secret key
#   WZBM_KEY_ID      - key id (required with WZBM_KEY_FILE / WZBM_KEY_ENV)
#   WZBM_MACOS_CANONICAL - ON: the exe entry uses the Mach-O canonical hash mode
#   WZBM_DATA_ARCHIVES   - OFF: the manifest covers only the executable

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED WZBM_DATA_ARCHIVES)
	set(WZBM_DATA_ARCHIVES ON)
endif()
set(_required_vars WZBM_EXE WZBM_OUTPUT WZBM_VERSION WZBM_COMMIT WZBM_PLATFORM)
if(WZBM_DATA_ARCHIVES)
	list(APPEND _required_vars WZBM_DATA_DIR)
else()
	get_filename_component(WZBM_DATA_DIR "${WZBM_EXE}" DIRECTORY)
endif()
foreach(_var IN LISTS _required_vars)
	if(NOT DEFINED ${_var})
		message(FATAL_ERROR "Missing required variable: ${_var}")
	endif()
endforeach()

# file(GLOB_RECURSE) ignores relative patterns in script mode
foreach(_var WZBM_EXE WZBM_DATA_DIR WZBM_OUTPUT)
	get_filename_component(${_var} "${${_var}}" ABSOLUTE)
endforeach()
if(WZBM_TOOL)
	get_filename_component(WZBM_TOOL "${WZBM_TOOL}" ABSOLUTE)
endif()
if(NOT WZBM_TOOL AND WZBM_MACOS_CANONICAL)
	message(FATAL_ERROR "WZBM_MACOS_CANONICAL requires WZBM_TOOL")
endif()
if(NOT WZBM_TOOL AND (DEFINED WZBM_KEY_FILE OR DEFINED WZBM_KEY_ENV))
	message(FATAL_ERROR "Signing requires WZBM_TOOL")
endif()
if((DEFINED WZBM_KEY_FILE OR DEFINED WZBM_KEY_ENV) AND NOT DEFINED WZBM_KEY_ID)
	message(FATAL_ERROR "WZBM_KEY_FILE / WZBM_KEY_ENV requires WZBM_KEY_ID")
endif()
if(DEFINED WZBM_KEY_FILE AND DEFINED WZBM_KEY_ENV)
	message(FATAL_ERROR "Specify only one of WZBM_KEY_FILE / WZBM_KEY_ENV")
endif()

if(NOT DEFINED WZBM_PUBLISHER)
	set(WZBM_PUBLISHER "self-built")
endif()
set(_copyright_year 2026)
if(WZBM_COPYRIGHT_YEAR MATCHES "^[0-9]+$" AND WZBM_COPYRIGHT_YEAR GREATER _copyright_year)
	set(_copyright_year "${WZBM_COPYRIGHT_YEAR}")
endif()
if(NOT DEFINED WZBM_NOTICE)
	set(WZBM_NOTICE "This file identifies a build of Warzone 2100, a free and open-source game, developed by the Warzone 2100 Project @ https://github.com/Warzone2100")
endif()

function(json_escape out_var value)
	string(REPLACE "\\" "\\\\" value "${value}")
	string(REPLACE "\"" "\\\"" value "${value}")
	set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(run_hash_exe out_hash out_size file)
	if(NOT WZBM_TOOL)
		file(SHA256 "${file}" _hash)
		file(SIZE "${file}" _size)
		set(${out_hash} "${_hash}" PARENT_SCOPE)
		set(${out_size} "${_size}" PARENT_SCOPE)
		return()
	endif()
	execute_process(
		COMMAND "${WZBM_TOOL}" hash-exe "${file}"
		OUTPUT_VARIABLE _output
		RESULT_VARIABLE _result
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "hash-exe failed for: ${file}")
	endif()
	if(NOT _output MATCHES "^([0-9a-f]+)  ([0-9]+)$")
		message(FATAL_ERROR "Unexpected hash-exe output for: ${file}")
	endif()
	set(${out_hash} "${CMAKE_MATCH_1}" PARENT_SCOPE)
	set(${out_size} "${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

# exe entry
if(WZBM_MACOS_CANONICAL)
	execute_process(
		COMMAND "${WZBM_TOOL}" hash-exe "${WZBM_EXE}" --canonical
		OUTPUT_VARIABLE _canonical_output
		RESULT_VARIABLE _result
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "hash-exe --canonical failed for: ${WZBM_EXE}")
	endif()
	string(REPLACE "\n" ";" _canonical_lines "${_canonical_output}")
	set(_slices "")
	foreach(_line IN LISTS _canonical_lines)
		if(NOT _line MATCHES "^([^ ]+)  ([0-9a-f]+)$")
			message(FATAL_ERROR "Unexpected hash-exe --canonical output line: ${_line}")
		endif()
		if(_slices)
			string(APPEND _slices ",")
		endif()
		string(APPEND _slices "\n   \"${CMAKE_MATCH_1}\": \"${CMAKE_MATCH_2}\"")
	endforeach()
	set(_exe_entry "{ \"hash_mode\": \"macho-canonical\", \"slices\": {${_slices}\n  } }")
else()
	run_hash_exe(_exe_hash _exe_size "${WZBM_EXE}")
	set(_exe_entry "{ \"sha256\": \"${_exe_hash}\", \"size\": ${_exe_size} }")
endif()

# core data archives
set(_wz_files "")
if(WZBM_DATA_ARCHIVES)
	set(_wz_files base.wz mp.wz)
	foreach(_wz IN LISTS _wz_files)
		if(NOT EXISTS "${WZBM_DATA_DIR}/${_wz}")
			message(FATAL_ERROR "Missing core data archive: ${WZBM_DATA_DIR}/${_wz}")
		endif()
	endforeach()
endif()
set(_artifacts " \"exe\": ${_exe_entry}")
foreach(_wz IN LISTS _wz_files)
	run_hash_exe(_wz_hash _wz_size "${WZBM_DATA_DIR}/${_wz}")
	json_escape(_wz_escaped "${_wz}")
	string(APPEND _artifacts ",\n  \"${_wz_escaped}\": { \"sha256\": \"${_wz_hash}\", \"size\": ${_wz_size} }")
endforeach()

string(TIMESTAMP _issued "%Y-%m-%dT%H:%M:%SZ" UTC)

json_escape(_publisher "${WZBM_PUBLISHER}")
json_escape(_version "${WZBM_VERSION}")
json_escape(_commit "${WZBM_COMMIT}")
json_escape(_platform "${WZBM_PLATFORM}")
json_escape(_notice "${WZBM_NOTICE}")

set(_attribution "")
string(APPEND _attribution "  \"notice\": \"${_notice}\",\n")
string(APPEND _attribution "  \"copyright\": \"Copyright (C) 2005-${_copyright_year} Warzone 2100 Project, (C) 1999-2004 Eidos Interactive\",\n")
string(APPEND _attribution "  \"license\": \"GPL-2.0-or-later\",\n")
string(APPEND _attribution "  \"website\": \"https://wz2100.net\"")
if(DEFINED WZBM_CONTACT)
	json_escape(_contact "${WZBM_CONTACT}")
	string(APPEND _attribution ",\n  \"contact\": \"${_contact}\"")
endif()

set(_manifest "")
string(APPEND _manifest "{\n")
string(APPEND _manifest " \"schema\": 1,\n")
string(APPEND _manifest " \"publisher\": \"${_publisher}\",\n")
string(APPEND _manifest " \"project\": \"Warzone 2100\",\n")
string(APPEND _manifest " \"version\": \"${_version}\",\n")
string(APPEND _manifest " \"commit\": \"${_commit}\",\n")
if(NOT "${WZBM_ORIGIN}" STREQUAL "")
	json_escape(_origin "${WZBM_ORIGIN}")
	string(APPEND _manifest " \"origin\": \"${_origin}\",\n")
endif()
string(APPEND _manifest " \"platform\": \"${_platform}\",\n")
string(APPEND _manifest " \"artifacts\": {\n ${_artifacts}\n },\n")
string(APPEND _manifest " \"attribution\": {\n${_attribution}\n },\n")
string(APPEND _manifest " \"issued\": \"${_issued}\"\n")
string(APPEND _manifest "}")

if(DEFINED WZBM_KEY_FILE OR DEFINED WZBM_KEY_ENV)
	if(DEFINED WZBM_KEY_ENV)
		set(_key_args --key-env "${WZBM_KEY_ENV}")
	else()
		set(_key_args --key "${WZBM_KEY_FILE}")
	endif()
	set(_manifest_file "${WZBM_OUTPUT}.manifest.tmp")
	file(WRITE "${_manifest_file}" "${_manifest}")
	execute_process(
		COMMAND "${WZBM_TOOL}" sign
			--manifest "${_manifest_file}"
			${_key_args}
			--key-id "${WZBM_KEY_ID}"
			--out "${WZBM_OUTPUT}"
			--artifacts-dir "${WZBM_DATA_DIR}"
			--exe "${WZBM_EXE}"
		RESULT_VARIABLE _result
	)
	file(REMOVE "${_manifest_file}")
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "Signing failed")
	endif()
	message(STATUS "Wrote signed build manifest: ${WZBM_OUTPUT}")
else()
	file(WRITE "${WZBM_OUTPUT}" "${_manifest}")
	message(STATUS "Wrote unsigned build manifest: ${WZBM_OUTPUT}")
endif()
