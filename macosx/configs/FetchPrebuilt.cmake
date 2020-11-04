#
# Required input defines:
#   - FILENAME
#   - URL
#   - EXPECTED_SHA256
#   - OUT_DIR
#
#######################################################################
#
# Note:
#
# This contains several functions adapted from CMake modules licensed
# under the CMake license (https://cmake.org/licensing)
# (See below.)
#
########################################################################
#
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2019 Kitware, Inc. and Contributors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of Contributors
#   may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#######################################################################
#

set(REQUIRED_INPUT_DEFINES FILENAME URL EXPECTED_SHA256 OUT_DIR)
foreach(_input_define ${REQUIRED_INPUT_DEFINES})
	if(NOT DEFINED ${_input_define} OR "${${_input_define}}" STREQUAL "")
		message( FATAL_ERROR "Missing required input define: ${_input_define}" )
	endif()
endforeach()

#######################################################################

set(_dl_directory "macosx/prebuilt")
set(_extract_directory "macosx/external")

get_filename_component(_full_extract_directory "${_extract_directory}/${OUT_DIR}" ABSOLUTE)

#######################################################################

if(EXISTS "${_full_extract_directory}/.SHA256SumLoc")
	file(READ "${_full_extract_directory}/.SHA256SumLoc" _strings_existing_sha256 ENCODING UTF-8)
	if(_strings_existing_sha256 STREQUAL EXPECTED_SHA256)
		# Already have expected download - skip
		message(STATUS "Already downloaded: ${_full_extract_directory}; skipping")
		return()
	else()
		# Not the expected version / content - remove the existing extracted directory
		file(REMOVE_RECURSE "${_full_extract_directory}")
	endif()
	unset(_strings_existing_sha256)
endif()

set(_download_file TRUE)
if(EXISTS "${_dl_directory}/${FILENAME}")
	message(STATUS "Verifying existing download: ${FILENAME}")
	file(SHA256 "${_dl_directory}/${FILENAME}" _dl_hash)
	if(_dl_hash STREQUAL EXPECTED_SHA256)
		# Skip download (re-use existing)
		message(STATUS "Re-using existing download: ${FILENAME}")
		set(_download_file FALSE)
	else()
		# Remove existing (outdated) download
		message(STATUS "Removing outdated (or corrupt) existing download: ${FILENAME}")
		file(REMOVE "${_dl_directory}/${FILENAME}")
	endif()
endif()

#######################################################################
# Download file (if needed)

if(_download_file)
	
	message(STATUS "Downloading: ${URL}")

	# FileDownloadRetry adapted from: https://github.com/Kitware/CMake/blob/master/Modules/ExternalProject-download.cmake.in
	function(FileDownloadRetry _url _destination _expectedSHA256Hash)
		set(retry_number 3)
		set(_dl_attempts 0)
		set(_dl_success FALSE)
		get_filename_component(_dl_filename "${_destination}" NAME)
		while(NOT _dl_success AND _dl_attempts LESS ${retry_number})
			if(EXISTS "${_destination}")
				file(REMOVE "${_destination}")
			endif()
			file(
				DOWNLOAD "${_url}" "${_destination}"
				INACTIVITY_TIMEOUT 60
				SHOW_PROGRESS
				TLS_VERIFY ON
				STATUS _dl_status
				LOG _dl_log
			)
			list(GET _dl_status 0 _dl_status_code)
			list(GET _dl_status 1 _dl_status_string)
			if (_dl_status_code EQUAL 0)
				# Check hash
				message(STATUS "Verifying download: ${_dl_filename}")
				file(SHA256 "${_destination}" _dl_hash)
				if(NOT _dl_hash STREQUAL _expectedSHA256Hash)
					message(STATUS "SHA256 does not match for \"${_destination}\": (received: ${_dl_hash}) (expecting: ${_expectedSHA256Hash}); removing...")
				else()
					# Successful download
					set(_dl_success TRUE)
				endif()
			else()
				MATH(EXPR _num_attempt "${_dl_attempts}+1")
				string(APPEND _logDLFailures "[Attempt ${_num_attempt}] has failed\n")
				string(APPEND _logDLFailures "status_code: ${_dl_status_code}\n")
				string(APPEND _logDLFailures "status_string: ${_dl_status_string}\n")
				string(APPEND _logDLFailures "--- LOG BEGIN ---\n")
				string(APPEND _logDLFailures "${_dl_log}\n")
				string(APPEND _logDLFailures "--- LOG END ---\n\n")
				message(STATUS "Error (${_dl_status_code}); Attempt (${_num_attempt})...")
			endif()
			MATH(EXPR _dl_attempts "${_dl_attempts}+1")
		endwhile()
	
		if(_dl_success)
			message(STATUS "Download successful")
		else()
			message(FATAL_ERROR "Failed to download: ${_url}\n${_logDLFailures}")
		endif()
	endfunction()
	
	FileDownloadRetry("${URL}" "${_dl_directory}/${FILENAME}" "${EXPECTED_SHA256}")
	
endif()

#######################################################################
# Extracting

function(ExtractDMG filename directory)

	if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
		message(FATAL_ERROR "Extraction of DMG is only supported on macOS")
	endif()

	# Make file names absolute:
	#
	get_filename_component(filename "${filename}" ABSOLUTE)
	get_filename_component(directory "${directory}" ABSOLUTE)

	find_program(HDIUTIL_COMMAND hdiutil REQUIRED)
	find_program(RSYNC_COMMAND rsync REQUIRED)

	if(NOT EXISTS "${filename}")
		message(FATAL_ERROR "error: file to extract does not exist: '${filename}'")
	endif()

	get_filename_component(_filename_dir "${filename}" DIRECTORY)
	set(_tmp_dmg_mount_path "${_filename_dir}/.mountedDMGs")

	file(MAKE_DIRECTORY "${_tmp_dmg_mount_path}")

	# mount the DMG to the temp (local) path
	get_filename_component(_filename_name_only "${filename}" NAME)
	set(_MountPoint "${_tmp_dmg_mount_path}/${_filename_name_only}")

	message(STATUS "info: Mounting DMG: \"${filename}\" -> \"${_MountPoint}\"")
	execute_process(
		COMMAND ${HDIUTIL_COMMAND} attach -mountpoint "${_MountPoint}" "${filename}"
		OUTPUT_QUIET
		RESULT_VARIABLE rv
	)

	if(NOT rv EQUAL 0)
		message(FATAL_ERROR "error: hdiutil failed to mount the DMG: '${filename}'")
	endif()

	# copy the contents of the DMG to the expected "extraction" directory
	# exclude anything beginning with "." in the root directory to avoid errors (ex. ".Trash")
	message(STATUS "info: Copying DMG contents...")
	file(MAKE_DIRECTORY "${directory}")
	execute_process(
		COMMAND ${RSYNC_COMMAND} -av --exclude='.*' "${_MountPoint}/" "${directory}"
		OUTPUT_QUIET
		RESULT_VARIABLE rv
	)

	message(STATUS "info: Copying DMG contents... done")

	# unmount the DMG
	execute_process(
		COMMAND ${HDIUTIL_COMMAND} detach "${_MountPoint}"
		OUTPUT_QUIET
	)

	message(STATUS "info: Unmounting DMG... done")

endfunction()

# ExtractFile adapted from: https://github.com/Kitware/CMake/blob/master/Modules/ExternalProject.cmake
function(ExtractFile filename directory)

	set(_tar_args "")
	
	if(filename MATCHES "(\\.|=)(7z|tar\\.bz2|tar\\.gz|tar\\.xz|tbz2|tgz|txz|zip)$")
		set(_tar_args xfz)
	endif()

	if(filename MATCHES "(\\.|=)tar$")
		set(_tar_args xf)
	endif()

	if(_tar_args STREQUAL "")
		message(FATAL_ERROR "error: do not know how to extract '${filename}' -- known types are .7z, .tar, .tar.bz2, .tar.gz, .tar.xz, .tbz2, .tgz, .txz and .zip")
		return()
	endif()

	# Make file names absolute:
	#
	get_filename_component(filename "${filename}" ABSOLUTE)
	get_filename_component(directory "${directory}" ABSOLUTE)

	message(STATUS "extracting...
	     src='${filename}'
	     dst='${directory}'")

	if(NOT EXISTS "${filename}")
	  message(FATAL_ERROR "error: file to extract does not exist: '${filename}'")
	endif()

	# Prepare a space for extracting:
	#
	get_filename_component(tmpdir_name "${filename}" NAME_WE)
	set(i 1234)
	while(EXISTS "${directory}/../ex-${tmpdir_name}${i}")
	  math(EXPR i "${i} + 1")
	endwhile()
	set(ut_dir "${directory}/../ex-${tmpdir_name}${i}")
	file(MAKE_DIRECTORY "${ut_dir}")

	# Extract it:
	#
	message(STATUS "extracting... [tar ${_tar_args}]")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar ${_tar_args} ${filename}
		WORKING_DIRECTORY "${ut_dir}"
		RESULT_VARIABLE rv
	)

	if(NOT rv EQUAL 0)
		message(STATUS "extracting... [error clean up]")
		file(REMOVE_RECURSE "${ut_dir}")
		message(FATAL_ERROR "error: extract of '${filename}' failed")
	endif()

	# Analyze what came out of the tar file:
	#
	message(STATUS "extracting... [analysis]")
	file(GLOB contents "${ut_dir}/*")
	list(REMOVE_ITEM contents "${ut_dir}/.DS_Store")
	list(LENGTH contents n)
	if(NOT n EQUAL 1 OR NOT IS_DIRECTORY "${contents}")
	  set(contents "${ut_dir}")
	endif()

	# Move "the one" directory to the final directory:
	#
	message(STATUS "extracting... [rename]")
	file(REMOVE_RECURSE "${directory}")
	get_filename_component(contents "${contents}" ABSOLUTE)
	file(RENAME "${contents}" "${directory}")

	# Clean up:
	#
	message(STATUS "extracting... [clean up]")
	file(REMOVE_RECURSE "${ut_dir}")
	message(STATUS "extracting... done")

endfunction()

if(FILENAME MATCHES "(\\.|=)dmg$")
	ExtractDMG("${_dl_directory}/${FILENAME}" "${_full_extract_directory}")
else()
	ExtractFile("${_dl_directory}/${FILENAME}" "${_full_extract_directory}")
endif()

# Save the SHA256
file(WRITE "${_full_extract_directory}/.SHA256SumLoc" "${EXPECTED_SHA256}")

#######################################################################

message(STATUS "Downloaded and extracted to: ${_full_extract_directory}")
