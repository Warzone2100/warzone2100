# Generates the build manifest for a macOS .app bundle, for packaging flows that assemble it outside of CPack.
#
# Writes the manifest into Contents/Resources, re-signs the bundle, and asserts the canonical executable
# hash is unchanged across the re-signing.
#
# Usage: cmake -DWZBM_...=... -P buildmanifest_bundle.cmake
#
# Required:
#   WZBM_TOOL         - path to the wzbuildmanifest executable
#   WZBM_APP_BUNDLE   - path to the .app bundle
#   WZBM_REPO_DIR     - repo checkout (revision info + the generation script)
#   WZBM_PLATFORM     - platform artifact identifier
# Optional:
#   WZBM_PUBLISHER    - publisher id (default: self-built)
#   WZBM_ENTITLEMENTS - entitlements plist for the re-signing pass
#   WZBM_IDENTITY     - code signing identity (default: ad-hoc)

cmake_minimum_required(VERSION 3.19)

foreach(_var WZBM_TOOL WZBM_APP_BUNDLE WZBM_REPO_DIR WZBM_PLATFORM)
	if(NOT DEFINED ${_var})
		message(FATAL_ERROR "Missing required variable: ${_var}")
	endif()
endforeach()
foreach(_var WZBM_TOOL WZBM_APP_BUNDLE WZBM_REPO_DIR)
	get_filename_component(${_var} "${${_var}}" ABSOLUTE)
endforeach()
if(NOT DEFINED WZBM_IDENTITY)
	set(WZBM_IDENTITY "-")
endif()

function(canonical_hashes out_var file)
	execute_process(
		COMMAND "${WZBM_TOOL}" hash-exe "${file}" --canonical
		OUTPUT_VARIABLE _output
		RESULT_VARIABLE _result
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "hash-exe --canonical failed for: ${file}")
	endif()
	set(${out_var} "${_output}" PARENT_SCOPE)
endfunction()

file(GLOB _bundle_exes LIST_DIRECTORIES false "${WZBM_APP_BUNDLE}/Contents/MacOS/*")
list(LENGTH _bundle_exes _num_bundle_exes)
if(NOT _num_bundle_exes EQUAL 1)
	message(FATAL_ERROR "Expected exactly one file in Contents/MacOS, found: ${_bundle_exes}")
endif()
list(GET _bundle_exes 0 _bundle_exe)
get_filename_component(_bundle_exe_name "${_bundle_exe}" NAME)

set(_resources_dir "${WZBM_APP_BUNDLE}/Contents/Resources")
set(_data_dir "${_resources_dir}/data")
if(NOT IS_DIRECTORY "${_data_dir}")
	message(FATAL_ERROR "No data directory in the bundle: ${_data_dir}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -DSKIPUPDATECACHE="1" -DVAROUT=1 -P "${WZBM_REPO_DIR}/build_tools/autorevision.cmake"
	WORKING_DIRECTORY "${WZBM_REPO_DIR}"
	OUTPUT_VARIABLE autorevision_info
	OUTPUT_STRIP_TRAILING_WHITESPACE
	RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
	message(FATAL_ERROR "Unable to obtain revision info from: ${WZBM_REPO_DIR}")
endif()
include("${WZBM_REPO_DIR}/build_tools/autorevision_helpers.cmake")
cmakeSetAutorevisionValues("${autorevision_info}")
if(NOT VCS_FULL_HASH)
	message(FATAL_ERROR "Unable to obtain revision info from: ${WZBM_REPO_DIR}")
endif()
if(VCS_TAG)
	set(_wzbm_version "${VCS_TAG}")
else()
	set(_wzbm_version "${VCS_BRANCH} ${VCS_SHORT_HASH}")
endif()

canonical_hashes(_canonical_before "${_bundle_exe}")

set(_wzbm_args
	"-DWZBM_TOOL=${WZBM_TOOL}"
	"-DWZBM_EXE=${_bundle_exe}"
	"-DWZBM_DATA_DIR=${_data_dir}"
	"-DWZBM_OUTPUT=${_resources_dir}/${_bundle_exe_name}.buildmanifest"
	"-DWZBM_VERSION=${_wzbm_version}"
	"-DWZBM_COMMIT=${VCS_FULL_HASH}"
	"-DWZBM_ORIGIN=${VCS_ORIGIN}"
	"-DWZBM_COPYRIGHT_YEAR=${VCS_MOST_RECENT_COMMIT_YEAR}"
	"-DWZBM_PLATFORM=${WZBM_PLATFORM}"
	"-DWZBM_MACOS_CANONICAL=ON"
)
if(NOT "${WZBM_PUBLISHER}" STREQUAL "")
	list(APPEND _wzbm_args "-DWZBM_PUBLISHER=${WZBM_PUBLISHER}")
endif()
# Signing key (from the packaging environment, ex. CI secrets)
if(NOT "$ENV{WZ_BUILDMANIFEST_KEY_ID}" STREQUAL "")
	if(NOT "$ENV{WZ_BUILDMANIFEST_KEY}" STREQUAL "")
		list(APPEND _wzbm_args "-DWZBM_KEY_ENV=WZ_BUILDMANIFEST_KEY")
		list(APPEND _wzbm_args "-DWZBM_KEY_ID=$ENV{WZ_BUILDMANIFEST_KEY_ID}")
	elseif(NOT "$ENV{WZ_BUILDMANIFEST_KEY_FILE}" STREQUAL "")
		list(APPEND _wzbm_args "-DWZBM_KEY_FILE=$ENV{WZ_BUILDMANIFEST_KEY_FILE}")
		list(APPEND _wzbm_args "-DWZBM_KEY_ID=$ENV{WZ_BUILDMANIFEST_KEY_ID}")
	endif()
endif()

execute_process(
	COMMAND ${CMAKE_COMMAND} ${_wzbm_args} -P "${WZBM_REPO_DIR}/pkg/buildmanifest_generate.cmake"
	RESULT_VARIABLE _wzbm_result
)
if(NOT _wzbm_result EQUAL 0)
	message(FATAL_ERROR "Build manifest generation failed")
endif()

set(_codesign_args --force --options runtime -s "${WZBM_IDENTITY}")
if(NOT "${WZBM_ENTITLEMENTS}" STREQUAL "")
	list(APPEND _codesign_args --entitlements "${WZBM_ENTITLEMENTS}")
endif()
execute_process(
	COMMAND codesign ${_codesign_args} "${WZBM_APP_BUNDLE}"
	RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
	message(FATAL_ERROR "codesign failed after writing the manifest")
endif()

canonical_hashes(_canonical_after "${_bundle_exe}")
if(NOT _canonical_before STREQUAL _canonical_after)
	message(FATAL_ERROR "Canonical executable hash changed across re-signing.\nBefore:\n${_canonical_before}\nAfter:\n${_canonical_after}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "Canonical executable hash unchanged across re-signing")
