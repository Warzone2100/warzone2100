# Signs the build manifest inside an extracted package tree, replacing it with the signed envelope.
#
# The manifest's artifact hashes are re-verified against the packaged files before anything is signed.
#
# Usage: cmake -DWZBM_...=... -P buildmanifest_sign_package.cmake
#
# Required:
#   WZBM_TOOL        - path to the wzbuildmanifest executable
#   WZBM_PACKAGE_DIR - root of the extracted package
#   WZBM_KEY_ID      - key id
#
# The secret key is read from the WZ_BUILDMANIFEST_KEY environment variable.

cmake_minimum_required(VERSION 3.19)

foreach(_var WZBM_TOOL WZBM_PACKAGE_DIR WZBM_KEY_ID)
	if(NOT DEFINED ${_var})
		message(FATAL_ERROR "Missing required variable: ${_var}")
	endif()
endforeach()
if("$ENV{WZ_BUILDMANIFEST_KEY}" STREQUAL "")
	message(FATAL_ERROR "WZ_BUILDMANIFEST_KEY is not set in the environment")
endif()
foreach(_var WZBM_TOOL WZBM_PACKAGE_DIR)
	get_filename_component(${_var} "${${_var}}" ABSOLUTE)
endforeach()

file(GLOB_RECURSE _manifests LIST_DIRECTORIES false "${WZBM_PACKAGE_DIR}/*.buildmanifest")
list(LENGTH _manifests _num_manifests)
if(NOT _num_manifests EQUAL 1)
	message(FATAL_ERROR "Expected exactly one manifest in the package, found: ${_manifests}")
endif()
list(GET _manifests 0 _manifest)
get_filename_component(_manifest_dir "${_manifest}" DIRECTORY)
get_filename_component(_exe_basename "${_manifest}" NAME_WLE)

set(_exe "")
foreach(_candidate "${_manifest_dir}/${_exe_basename}.exe" "${_manifest_dir}/${_exe_basename}")
	if(EXISTS "${_candidate}" AND NOT IS_DIRECTORY "${_candidate}")
		set(_exe "${_candidate}")
		break()
	endif()
endforeach()
if(NOT _exe)
	# Linux packages keep the manifest under lib/, apart from the executable
	file(GLOB_RECURSE _exes LIST_DIRECTORIES false "${WZBM_PACKAGE_DIR}/${_exe_basename}.exe" "${WZBM_PACKAGE_DIR}/${_exe_basename}")
	list(LENGTH _exes _num_exes)
	if(NOT _num_exes EQUAL 1)
		message(FATAL_ERROR "Expected exactly one ${_exe_basename} executable in the package, found: ${_exes}")
	endif()
	list(GET _exes 0 _exe)
endif()

file(GLOB_RECURSE _staged_basewz LIST_DIRECTORIES false "${WZBM_PACKAGE_DIR}/base.wz")
list(LENGTH _staged_basewz _num_staged_basewz)
if(NOT _num_staged_basewz EQUAL 1)
	message(FATAL_ERROR "Expected exactly one base.wz in the package, found: ${_staged_basewz}")
endif()
list(GET _staged_basewz 0 _staged_basewz)
get_filename_component(_data_dir "${_staged_basewz}" DIRECTORY)

set(_signed "${_manifest}.signed.tmp")
execute_process(
	COMMAND "${WZBM_TOOL}" sign
		--manifest "${_manifest}"
		--key-env WZ_BUILDMANIFEST_KEY
		--key-id "${WZBM_KEY_ID}"
		--out "${_signed}"
		--artifacts-dir "${_data_dir}"
		--exe "${_exe}"
	RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
	file(REMOVE "${_signed}")
	message(FATAL_ERROR "Signing failed for: ${_manifest}")
endif()
file(RENAME "${_signed}" "${_manifest}")
file(CHMOD "${_manifest}" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
message(STATUS "Signed the build manifest in: ${WZBM_PACKAGE_DIR}")
