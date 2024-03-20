#
# _WZ_BASIS_ENCODE_GET_UNIQUE_TARGET_NAME is adapted from
# _GETTEXT_GET_UNIQUE_TARGET_NAME in: https://github.com/Kitware/CMake/blob/master/Modules/FindGettext.cmake
#
# Original license:
# -------------------------------------------------------------------
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2023 Kitware, Inc. and Contributors
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
# -------------------------------------------------------------------

function(_WZ_BASIS_ENCODE_GET_UNIQUE_TARGET_NAME _name _unique_name)
	set(propertyName "_WZ_BASIS_ENCODE_UNIQUE_COUNTER_${_name}")
	get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
	if(NOT currentCounter)
		set(currentCounter 1)
	endif()
	set(${_unique_name} "${_name}_${currentCounter}" PARENT_SCOPE)
	math(EXPR currentCounter "${currentCounter} + 1")
	set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter} )
endfunction()

########################################################################
#
# WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR outputDir
#			   TYPE <TEXTURE, NORMALMAP, SPECULARMAP, ALPHAMASK, UITEXTURE>
#			   [RESIZE <512 | 1024 | 2048 ...>]
#			   [UASTC_LEVEL <0 | 1 | 2 | 3 | 4> = 2]
#			   [RDO]
#			   FILES [files...]
#			   ENCODING_TARGET [target name]
#			   [TARGET_FOLDER <folder>]
#			   [ALL])
#
# Basis-encode a list of texture files, into the output folder <outputDir>.
#
# TYPE must be specified, and must be one of "TEXTURE", "NORMALMAP", "SPECULARMAP", or "ALPHAMASK".
#
# Notes on expected input formats:
# - For "SPECULARMAP": a single-channel (grayscale) PNG. Only the first channel is extracted and used.
# - For "ALPHAMASK": an RGBA PNG. Only the alpha channel is extracted and used.
#
# If RESIZE is specified, the texture images will be resized to RESIZE x RESIZE dimensions (if they aren't already) as a first step.
#
# If UASTC_LEVEL is specified, the -uastc_level parameter will be specified to set the UASTC encoding level:
# > Range is [0,4], default is 2, higher=slower but higher quality.
# > 0=fastest/lowest quality, 3=slowest practical option, 4=impractically slow/highest achievable quality
#
function(WZ_BASIS_ENCODE_TEXTURES)

	if(NOT DEFINED BASIS_UNIVERSAL_CLI)
		message(FATAL_ERROR "No basisu tool has been provided - set BASIS_UNIVERSAL_CLI to the path to basisu or disable WZ_ENABLE_BASIS_UNIVERSAL!")
	endif()

	set(_options ALL RDO)
	set(_oneValueArgs OUTPUT_DIR TYPE RESIZE ENCODING_TARGET TARGET_FOLDER UASTC_LEVEL)
	set(_multiValueArgs FILES)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	# Check that mandatory parameters were provided
	if(NOT _parsedArguments_OUTPUT_DIR)
		message( FATAL_ERROR "Missing required OUTPUT_DIR parameter" )
	endif()
	if(NOT _parsedArguments_TYPE OR NOT _parsedArguments_TYPE MATCHES "^(TEXTURE|NORMALMAP|SPECULARMAP|ALPHAMASK|UITEXTURE)$")
		message( FATAL_ERROR "Missing valid TYPE parameter" )
	endif()
	if(NOT _parsedArguments_ENCODING_TARGET)
		message( FATAL_ERROR "Missing required ENCODING_TARGET parameter" )
	endif()
	if(NOT _parsedArguments_FILES)
		message( FATAL_ERROR "Missing required FILES parameter" )
	endif()

	# Optional arguments with defaults
	if(DEFINED _parsedArguments_UASTC_LEVEL)
		if (NOT _parsedArguments_UASTC_LEVEL MATCHES "^(0|1|2|3|4)$")
			message( FATAL_ERROR "Invalid UASTC_LEVEL value: ${_parsedArguments_UASTC_LEVEL}" )
		endif()
		set(_uastc_level ${_parsedArguments_UASTC_LEVEL})
	else()
		set(_uastc_level 2)
	endif()

	# Construct variable encoding arguments
	unset(_resample_arguments)
	if(_parsedArguments_RESIZE)
		set(_resample_arguments "-resample" "${_parsedArguments_RESIZE}" "${_parsedArguments_RESIZE}")
	endif()
	unset(_rdo_arguments)
	unset(_type_dependent_arguments)
	if(_parsedArguments_TYPE STREQUAL "TEXTURE")
		set(_type_dependent_arguments -mipmap)
		if (_parsedArguments_RDO)
			set(_rdo_arguments -uastc_rdo_l 1.0)
		endif()
	elseif(_parsedArguments_TYPE STREQUAL "UITEXTURE")
		# no mipmaps
		if (_parsedArguments_RDO)
			set(_rdo_arguments -uastc_rdo_l 1.0)
		endif()
	elseif(_parsedArguments_TYPE STREQUAL "NORMALMAP")
		set(_type_dependent_arguments -mipmap -normal_map -mip_filter mitchell)
		if (_parsedArguments_RDO)
			set(_rdo_arguments -uastc_rdo_l 1.0)
		endif()
	elseif(_parsedArguments_TYPE STREQUAL "SPECULARMAP")
		set(_type_dependent_arguments -mipmap -linear -mip_linear -no_selector_rdo -no_endpoint_rdo -swizzle rrra -no_alpha -mip_filter mitchell)
		if (_parsedArguments_RDO)
			set(_rdo_arguments -uastc_rdo_l 1.0)
		endif()
	elseif(_parsedArguments_TYPE STREQUAL "ALPHAMASK")
		set(_type_dependent_arguments -mipmap -linear -mip_linear -no_selector_rdo -no_endpoint_rdo -swizzle aaaa -no_alpha)
	endif()

	file(MAKE_DIRECTORY "${_parsedArguments_OUTPUT_DIR}")

	foreach(TEXTURE ${_parsedArguments_FILES})
		get_filename_component(TEXTURE_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(TEXTURE_FILE_NAME_WE ${TEXTURE} NAME_WE)
		set(_output_name "${TEXTURE_FILE_NAME_WE}.ktx2")
		add_custom_command(OUTPUT "${_parsedArguments_OUTPUT_DIR}/${_output_name}"
			COMMAND "${BASIS_UNIVERSAL_CLI}"
			ARGS -ktx2 -uastc -uastc_level ${_uastc_level} ${_rdo_arguments} -uastc_rdo_m ${_type_dependent_arguments} ${_resample_arguments} -output_file "${_parsedArguments_OUTPUT_DIR}/${_output_name}" -file "${TEXTURE}"
			DEPENDS "${TEXTURE}"
			VERBATIM
		)
		list(APPEND TEXTURE_LIST "${_parsedArguments_OUTPUT_DIR}/${_output_name}")
	endforeach()

	if(NOT TARGET ${_parsedArguments_ENCODING_TARGET})
		add_custom_target(${_parsedArguments_ENCODING_TARGET})
		set_property(TARGET ${_parsedArguments_ENCODING_TARGET} PROPERTY FOLDER "data")
	endif()

	_WZ_BASIS_ENCODE_GET_UNIQUE_TARGET_NAME("${_parsedArguments_ENCODING_TARGET}" uniqueTargetName)

	if(_parsedArguments_ALL)
		add_custom_target(${uniqueTargetName} ALL DEPENDS ${TEXTURE_LIST})
	else()
		add_custom_target(${uniqueTargetName} DEPENDS ${TEXTURE_LIST})
	endif()

	if (DEFINED _parsedArguments_TARGET_FOLDER)
		set_property(TARGET ${uniqueTargetName} PROPERTY FOLDER "${_parsedArguments_TARGET_FOLDER}")
	endif()

	add_dependencies(${_parsedArguments_ENCODING_TARGET} ${uniqueTargetName})

endfunction()
