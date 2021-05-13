if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	OPTION(WZ_INCLUDE_VIDEOS "Download & include videos in the application package" OFF)
endif()

find_package(ZIP REQUIRED)

find_program( GLSLC "glslc"
			  PATHS "$ENV{VULKAN_SDK}/bin"
)

file(GLOB VK_SHADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/generic.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx_text.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx_color.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/line.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_depth.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/button.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/texturedrect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/text.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terraindepth.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/water.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/button.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.frag"
)

set(SHADER_LIST "")
if(GLSLC)
	set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/base/shaders/vk")
	file(MAKE_DIRECTORY "${_output_dir}")
	foreach(SHADER ${VK_SHADERS})
		get_filename_component(SHADER_FILE_PATH ${SHADER} DIRECTORY)
		get_filename_component(SHADER_FILE ${SHADER} NAME)
		set(_output_name "${SHADER_FILE}.spv")
		add_custom_command(OUTPUT "${_output_dir}/${_output_name}"
			COMMAND "${GLSLC}"
			ARGS "-c" "${SHADER}" "-o" "${_output_dir}/${_output_name}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk"
			DEPENDS "${SHADER}"
			VERBATIM
		)
		list(APPEND SHADER_LIST "${_output_dir}/${_output_name}")
	endforeach()
	add_custom_target(glsl_compilation DEPENDS ${SHADER_LIST})
	set_property(TARGET glsl_compilation PROPERTY FOLDER "data")
else()
	message(STATUS "Unable to find glslc")
endif()

set(_glsl_generatedfiles_PATHS)
if(TARGET glsl_compilation)
	set(_glsl_generatedfiles_PATHS
		PATHS
			"shaders"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/base"
	)
endif()

COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	COMPRESSION_LEVEL 0
	PATHS
		"palette.txt"
		"audio"
		"campaigns"
		"components"
		"effects"
		"features"
		"gamedesc.lev"
		"ruleset.json"
		"images"
		"messages"
		"misc"
		"fonts"
		"script"
		"sequenceaudio"
		"shaders"
		"stats"
		"structs"
		"texpages"
		"tileset"
		"wrf"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base"
	${_glsl_generatedfiles_PATHS}
	IGNORE_GIT
	QUIET
)

COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
	COMPRESSION_LEVEL 0
	PATHS
		"addon.lev"
		"ruleset.json"
		"anims"
		"challenges"
		"tests"
		"components"
		"effects"
		"messages"
		"multiplay"
		"stats"
		"structs"
		"wrf"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mp"
	IGNORE_GIT
	QUIET
)

if(WZ_INCLUDE_VIDEOS)
	set(_download_script_path "${PROJECT_SOURCE_DIR}/cmake/FetchURLRetry.cmake")
	set(_sequences_dl_filename "sequences.wz")
	set(_sequences_dl_urls "https://github.com/Warzone2100/wz-sequences/releases/download/v3/standard-quality-en-sequences.wz" "https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/standard-quality-en/sequences.wz")
	set(_sequences_dl_sha256 "142ae905be288cca33357a49f42b884c190e828fc0b1b1773ded5dff774f41a3")
	set(_sequences_output_dir "${CMAKE_CURRENT_BINARY_DIR}")
	set(_exstatus 1)
	foreach(_sequences_dl_url "${_sequences_dl_urls}")
		execute_process(
			COMMAND ${CMAKE_COMMAND}
					-DFILENAME=${_sequences_dl_filename}
					-DURL=${_sequences_dl_url}
					-DEXPECTED_SHA256=${_sequences_dl_sha256}
					-DOUT_DIR=${_sequences_output_dir}
					-P ${_download_script_path}
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
			RESULT_VARIABLE _exstatus
		)
		if(_exstatus EQUAL 0)
			# Success downloading
			message(STATUS "Downloaded sequences.wz (campaign videos)")
			break()
		endif()
	endforeach()
	if(NOT _exstatus EQUAL 0)
		message(FATAL_ERROR "Failed to download sequences.wz (campaign videos)")
	endif()
endif()

ADD_CUSTOM_TARGET(data ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/base.wz" "${CMAKE_CURRENT_BINARY_DIR}/mp.wz")
if(TARGET glsl_compilation)
	add_dependencies(data glsl_compilation)
endif()

set(DATA_FILES
	"${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	"${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
)
if(WZ_INCLUDE_VIDEOS)
	list(APPEND DATA_FILES "${CMAKE_CURRENT_BINARY_DIR}/sequences.wz")
endif()
install(FILES ${DATA_FILES}
	DESTINATION "${WZ_DATADIR}"
	COMPONENT Data
)

file(GLOB DATA_MUSIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/music/*.ogg" "${CMAKE_CURRENT_SOURCE_DIR}/music/albums/*/*.*")
foreach(_music_file ${DATA_MUSIC_FILES})
	file(RELATIVE_PATH _music_file_relative_path "${CMAKE_CURRENT_SOURCE_DIR}/music" "${_music_file}")
	get_filename_component(_music_file_subdir_path "${_music_file_relative_path}" DIRECTORY)
	install(FILES ${_music_file}
		DESTINATION "${WZ_DATADIR}/music/${_music_file_subdir_path}"
		COMPONENT Data
	)
endforeach()

set(DATA_FILES ${DATA_FILES} PARENT_SCOPE)
set(DATA_MUSIC_FILES ${DATA_MUSIC_FILES} PARENT_SCOPE)
set(DATA_MUSIC_BASE_SOURCEDIR "${CMAKE_CURRENT_SOURCE_DIR}/music" PARENT_SCOPE)
