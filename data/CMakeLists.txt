if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	OPTION(WZ_INCLUDE_VIDEOS "Download & include videos in the application package" OFF)
endif()

find_package(ZIP REQUIRED)

###########################
# GLSLC shader compilation

find_program( GLSLC "glslc"
			  PATHS "$ENV{VULKAN_SDK}/bin"
)

file(GLOB VK_SHADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/generic.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx_text.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx_color.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/line.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_depth.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/texturedrect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/text.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terraindepth.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/water.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.frag"
)

set(SHADER_LIST "")
if(GLSLC)
	message(STATUS "Found glslc: ${GLSLC}")
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

#########################################
# Texture pages basis-universal encoding

# default texpages path
set(_texpages_PATHS
	PATHS
		"texpages"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base"
)

if(WZ_ENABLE_BASIS_UNIVERSAL AND NOT WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)
	if(NOT DEFINED BASIS_UNIVERSAL_CLI)
		message(FATAL_ERROR "No basisu tool has been provided - set BASIS_UNIVERSAL_CLI to the path to basisu or disable WZ_ENABLE_BASIS_UNIVERSAL!")
	endif()

	file(GLOB TEXPAGES_TERRAIN
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-82-yellow-sand-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-83-red-earth-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-84-tiles-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-85-cliff-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-86-bottom-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-87-mud-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-88-grass-arizona.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-89-gray-rubble-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-90-green-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-91-tiles-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-92-medium-tiles-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-93-big-tiles-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-94-cliffs-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-95-brown-rocks-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-96-burned-ground-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-97-burned-tiles-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-98-bottom-urban.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-99-grass-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-100-rock-earth-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-101-cliff-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-102-bottom-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-103-tiles-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-104-snowgrass-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-105-snow-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-106-snowrock-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-107-brown-earth-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-108-snowcliff-rockies.png"
		"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-109-sand-arizona.png"
	)

	set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/base/texpages")
	file(MAKE_DIRECTORY "${_output_dir}")
	set(_terrain_max_size 1024)
	foreach(TEXTURE ${TEXPAGES_TERRAIN})
		get_filename_component(TEXTURE_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(TEXTURE_FILE_NAME_WE ${TEXTURE} NAME_WE)
		set(_output_name "${TEXTURE_FILE_NAME_WE}.ktx2")
		add_custom_command(OUTPUT "${_output_dir}/${_output_name}"
			COMMAND "${BASIS_UNIVERSAL_CLI}"
			ARGS -ktx2 -uastc -uastc_level 2 -uastc_rdo_l 1.0 -uastc_rdo_m -mipmap -resample ${_terrain_max_size} ${_terrain_max_size} -output_file "${_output_dir}/${_output_name}" -file "${TEXTURE}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages"
			DEPENDS "${TEXTURE}"
			VERBATIM
		)
		list(APPEND TEXTURE_LIST "${_output_dir}/${_output_name}")
	endforeach()
	add_custom_target(texture_encoding DEPENDS ${TEXTURE_LIST})
	set_property(TARGET texture_encoding PROPERTY FOLDER "data")

	set(PROCESSED_TEXTURE_FILES ${TEXPAGES_TERRAIN})

	file(GLOB_RECURSE ALL_TEXPAGES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/*.png" "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/compression_overrides.txt" "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/*.radar")
	list(APPEND ALL_TEXPAGES_unprocessed ${ALL_TEXPAGES})
	list(REMOVE_ITEM ALL_TEXPAGES_unprocessed ${PROCESSED_TEXTURE_FILES})
	foreach(TEXPAGE_FILE ${ALL_TEXPAGES_unprocessed})
		file(RELATIVE_PATH _output_name "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages" "${TEXPAGE_FILE}")
		message(STATUS "Copy unprocessed image file: ${_output_name}")
		add_custom_command(OUTPUT "${_output_dir}/${_output_name}"
			COMMAND "${CMAKE_COMMAND}"
			ARGS -E copy_if_different "${TEXPAGE_FILE}" "${_output_dir}/${_output_name}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages"
			DEPENDS "${TEXPAGE_FILE}"
			VERBATIM
		)
		list(APPEND TEXTURE_UNPROCESSED_LIST "${_output_dir}/${_output_name}")
	endforeach()

	add_custom_target(texture_staging DEPENDS ${TEXTURE_UNPROCESSED_LIST})
	set_property(TARGET texture_staging PROPERTY FOLDER "data")

	# set the appropriate texpages path
	set(_texpages_PATHS
		PATHS
			"texpages"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/base"
	)

else()
	if(WZ_ENABLE_BASIS_UNIVERSAL AND WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)
		message(STATUS "WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES is set, textures will not be compressed as part of build. This should only be used for selected CI runs.")
	endif()
endif()

#########################
# Compress data archives

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
		"script"
		"sequenceaudio"
		"shaders"
		"stats"
		"structs"
		"tileset"
		"wrf"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base"
	${_glsl_generatedfiles_PATHS}
	${_texpages_PATHS}
	BUILD_ALWAYS_TARGET data_base
	IGNORE_GIT
	QUIET
)

add_subdirectory(fonts fonts_staging)
if(NOT DEFINED wz2100_fonts_FILES)
	message(FATAL_ERROR "Failed to find font files")
endif()

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
	BUILD_ALWAYS_TARGET data_mp
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
	DEPENDS data_base data_mp)
if(TARGET glsl_compilation)
	add_dependencies(data_base glsl_compilation)
endif()
if(TARGET texture_encoding)
	add_dependencies(data_base texture_encoding)
endif()
if(TARGET texture_staging)
	add_dependencies(data_base texture_staging)
endif()

# Copy font files to build directory to support IDE build+run (from the build dir) scenarios
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/fonts")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fonts")
endif()
foreach(font_file ${wz2100_fonts_FILES})
    add_custom_command(
		TARGET data
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${font_file}" "${CMAKE_CURRENT_BINARY_DIR}/fonts"
		VERBATIM
	)
endforeach()

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

install(FILES
	${wz2100_fonts_FILES}
	COMPONENT Data DESTINATION "${WZ_DATADIR}/fonts"
)

file(GLOB DATA_MUSIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/music/*.opus" "${CMAKE_CURRENT_SOURCE_DIR}/music/albums/*/*.*")
foreach(_music_file ${DATA_MUSIC_FILES})
	file(RELATIVE_PATH _music_file_relative_path "${CMAKE_CURRENT_SOURCE_DIR}/music" "${_music_file}")
	get_filename_component(_music_file_subdir_path "${_music_file_relative_path}" DIRECTORY)
	install(FILES ${_music_file}
		DESTINATION "${WZ_DATADIR}/music/${_music_file_subdir_path}"
		COMPONENT Data
	)
endforeach()

set(DATA_FILES ${DATA_FILES} PARENT_SCOPE)
set(DATA_FONTS ${wz2100_fonts_FILES} PARENT_SCOPE)
set(DATA_MUSIC_FILES ${DATA_MUSIC_FILES} PARENT_SCOPE)
set(DATA_MUSIC_BASE_SOURCEDIR "${CMAKE_CURRENT_SOURCE_DIR}/music" PARENT_SCOPE)
