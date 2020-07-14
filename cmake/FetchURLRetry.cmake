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

get_filename_component(_dl_directory "${OUT_DIR}" ABSOLUTE)

#######################################################################

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

message(STATUS "Downloaded to: ${_dl_directory}/${FILENAME}")
