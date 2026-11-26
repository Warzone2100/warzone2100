# Checks that every Vulkan stage writing gl_Position ends main() with
# "gl_Position = wzFinalizeClip(gl_Position);",
# unless it is marked WZ_CLIP_NOT_FINAL (see data/base/shaders/vk/wz_clip_space.glsl).
#
# Usage: cmake -DSHADER_LIST_FILE=<file with one stage path per line> -DSTAMP_FILE=<stamp>
#              -P WZCheckVulkanShaderConventions.cmake

cmake_minimum_required(VERSION 3.16...3.31)

if(NOT DEFINED SHADER_LIST_FILE OR NOT DEFINED STAMP_FILE)
	message(FATAL_ERROR "Usage: cmake -DSHADER_LIST_FILE=<file> -DSTAMP_FILE=<file> -P WZCheckVulkanShaderConventions.cmake")
endif()

file(STRINGS "${SHADER_LIST_FILE}" _wz_shaders)

set(_wz_errors "")
set(_wz_finalize_regex "gl_Position[ \t]*=[ \t]*wzFinalizeClip[ \t]*\\([ \t]*gl_Position[ \t]*\\)[ \t]*;")

foreach(_wz_shader IN LISTS _wz_shaders)
	get_filename_component(_wz_ext "${_wz_shader}" LAST_EXT)
	if(NOT _wz_ext STREQUAL ".vert" AND NOT _wz_ext STREQUAL ".tese")
		continue()
	endif()
	get_filename_component(_wz_name "${_wz_shader}" NAME)
	file(READ "${_wz_shader}" _wz_src)

	if(_wz_src MATCHES "WZ_CLIP_NOT_FINAL")
		if(_wz_src MATCHES "wzFinalizeClip")
			string(APPEND _wz_errors "  ${_wz_name}: is marked WZ_CLIP_NOT_FINAL but calls wzFinalizeClip()\n")
		endif()
		continue()
	endif()

	# Ignore line comments (ex. a commented-out "//gl_Position.y *= -1.;")
	string(REGEX REPLACE "//[^\r\n]*" "" _wz_code "${_wz_src}")
	if(NOT _wz_code MATCHES "gl_Position")
		continue()
	endif()

	string(REGEX REPLACE "${_wz_finalize_regex}" "@WZ_FINALIZE_CLIP@" _wz_code "${_wz_code}")
	string(REGEX MATCHALL "@WZ_FINALIZE_CLIP@" _wz_finalize_calls "${_wz_code}")
	list(LENGTH _wz_finalize_calls _wz_finalize_count)
	if(_wz_finalize_count EQUAL 0)
		string(APPEND _wz_errors "  ${_wz_name}: writes gl_Position but does not end main() with \"gl_Position = wzFinalizeClip(gl_Position);\"\n")
		continue()
	elseif(_wz_finalize_count GREATER 1)
		string(APPEND _wz_errors "  ${_wz_name}: calls wzFinalizeClip() more than once\n")
		continue()
	endif()
	string(FIND "${_wz_code}" "@WZ_FINALIZE_CLIP@" _wz_finalize_pos)
	string(SUBSTRING "${_wz_code}" ${_wz_finalize_pos} -1 _wz_after)
	if(_wz_after MATCHES "gl_Position")
		string(APPEND _wz_errors "  ${_wz_name}: uses gl_Position after \"gl_Position = wzFinalizeClip(gl_Position);\" (it must be the last use)\n")
	endif()
endforeach()

if(NOT _wz_errors STREQUAL "")
	message(FATAL_ERROR
		"Vulkan shader convention check failed:\n${_wz_errors}"
		"See data/base/shaders/vk/wz_clip_space.glsl."
	)
endif()

file(TOUCH "${STAMP_FILE}")
