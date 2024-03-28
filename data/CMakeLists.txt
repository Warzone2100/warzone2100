if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	OPTION(WZ_INCLUDE_VIDEOS "Download & include videos in the application package" OFF)
endif()
OPTION(WZ_INCLUDE_TERRAIN_HIGH "Include high terrain texture pack" ON)
OPTION(WZ_DOWNLOAD_PREBUILT_PACKAGES "Download prebuilt texture packages (if OFF, will generate from scratch - this can take a while to encode textures)" ON)

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	# "Fake" COMPRESS_ZIP that just stages the needed files in folders
	include(EmscriptenCompressZip)
else()
	find_package(ZIP REQUIRED)
endif()

###########################
# Prebuilt package DL info

include("WZPrebuiltPackages.cmake")

# Function to handle downloading package
function(WZ_FETCH_PREBUILT_PACKAGE PACKAGE_URL PACKAGE_FILENAME PACKAGE_SHA512 OUTPUT_PATH STATUS)

	if (DEFINED WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR)
		# Try to copy the package from the supplied asset dir, if present (and it matches the expected hash)
		if (EXISTS "${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}")
			file(SHA512 "${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}" _dl_predownloaded_sha512)
			if("${_dl_predownloaded_sha512}" STREQUAL "${PACKAGE_SHA512}")
				# Copy the package to the desired output location
				execute_process(
					COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}" "${OUTPUT_PATH}"
					RESULT_VARIABLE _copy_result
				)
				if (_copy_result EQUAL 0)
					message(STATUS "Copied predownloaded package: ${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}, with hash: ${PACKAGE_SHA512}")
					set(${STATUS} TRUE PARENT_SCOPE)
					return()
				else()
					message(WARNING "Failed to copy predownloaded package: ${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}, with error: ${_copy_result} - falling back to download")
				endif()
			else()
				message(WARNING "Found predownloaded package: ${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME}, with unexpected hash: ${_dl_predownloaded_sha512} - falling back to download")
			endif()
		else()
			message(STATUS "WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR specified, but ${WZ_DOWNLOAD_PREBUILT_PACKAGES_ASSET_DIR}/${PACKAGE_FILENAME} does not exist - falling back to download")
		endif()
	endif()

	# Check if file already exists at output path
	if (EXISTS "${OUTPUT_PATH}")
		file(SHA512 "${OUTPUT_PATH}" _dl_received_sha512)
		if("${_dl_received_sha512}" STREQUAL "${PACKAGE_SHA512}")
			message(STATUS "Found: Already downloaded prebuilt package: ${OUTPUT_PATH}, with hash: ${PACKAGE_SHA512}")
			set(${STATUS} TRUE PARENT_SCOPE)
			return()
		endif()
	endif()

	# Try to download the prebuilt package
	file(DOWNLOAD "${PACKAGE_URL}" "${OUTPUT_PATH}" INACTIVITY_TIMEOUT 60 SHOW_PROGRESS STATUS _dl_status TLS_VERIFY ON)
	list(GET _dl_status 0 _dl_status_code)
	list(GET _dl_status 1 _dl_error_message)
	if ((NOT _dl_status_code EQUAL 0) OR (NOT EXISTS "${OUTPUT_PATH}"))
		# Download failed
		message(WARNING "Failed to download prebuilt package: ${PACKAGE_URL}, with error: ${_dl_error_message}")
		set(${STATUS} FALSE PARENT_SCOPE)
	else()
		# Download allegedly succeeded - check hash
		file(SHA512 "${OUTPUT_PATH}" _dl_received_sha512)
		if("${_dl_received_sha512}" STREQUAL "${PACKAGE_SHA512}")
			message(STATUS "Downloaded prebuilt package: ${PACKAGE_URL}, with hash: ${PACKAGE_SHA512}")
			set(${STATUS} TRUE PARENT_SCOPE)
		else()
			message(WARNING "Failed to download prebuilt package: ${PACKAGE_URL}, with unexpected hash: ${_dl_received_sha512}")
			set(${STATUS} FALSE PARENT_SCOPE)
		endif()
	endif()
endfunction()

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
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_combined.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_depth.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_depth_only.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water_high.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water_classic.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_depth_instanced.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/rect_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/texturedrect.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/gfx.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/text.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_combined_classic.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_combined_medium.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_combined_high.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terraindepth.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_depth_only.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/water.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water_high.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/terrain_water_classic.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/decals.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/nolight_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/tcmask_depth_instanced.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/skybox.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/quad_texture2d.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/quad_texture2d.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/quad_texture2darray.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/quad_texture2darray.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/world_to_screen.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/shaders/vk/world_to_screen.frag"
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

include(WZBasisEncode)

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

	# Normal Terrain
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
	if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
		set(_terrain_max_size 512)
	endif()

	WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "TEXTURE" RESIZE "${_terrain_max_size}" RDO ENCODING_TARGET texture_encoding TARGET_FOLDER data ALL FILES ${TEXPAGES_TERRAIN})
	set(PROCESSED_TEXTURE_FILES "${TEXPAGES_TERRAIN}")

	if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
		# Additional Normal Texpages (that are 1024x1024)
		file(GLOB TEXPAGES_BASE_1024
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-8-player-buildings-bases-rockies.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-8-player-buildings-bases-urban.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-8-player-buildings-bases.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-9-player-buildings-bases-rockies.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-9-player-buildings-bases-urban.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-9-player-buildings-bases.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-11-player-buildings.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-12-player-buildings.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-13-player-buildings.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-29-features-arizona.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-30-features-rockies.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-31-features-urban.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-34-buildings.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-111-player-buildings-nexus-bases.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-112-player-buildings-nexus.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-114-player-buildings_Collective.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-116-player-buildings_nex.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-117-player-buildings-bases_nex.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-120-player-buildings-bases_Collective.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-121-player-buildings_Collective.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-122-player-buildings_Collective.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/page-123-laboratories_Collective.png"
		)

		set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/base/texpages")
		file(MAKE_DIRECTORY "${_output_dir}")
		set(_texpage_max_size 1024)
		if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
			set(_texpage_max_size 512)
		endif()

		WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "TEXTURE" RESIZE "${_texpage_max_size}" RDO ENCODING_TARGET texture_encoding TARGET_FOLDER data ALL FILES ${TEXPAGES_BASE_1024})
		list(APPEND PROCESSED_TEXTURE_FILES "${TEXPAGES_BASE_1024}")
	endif()

	# Backdrops
	file(GLOB TEXPAGES_BACKDROPS_ALL
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/bdrops/*.png"
	)
	if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
		# limit backdrops included on Emscripten (to save space)
		file(GLOB TEXPAGES_BACKDROPS
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/bdrops/backdrop0.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/bdrops/missionend.png"
		)
	else()
		set(TEXPAGES_BACKDROPS ${TEXPAGES_BACKDROPS_ALL})
	endif()

	set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/base/texpages/bdrops")
	file(MAKE_DIRECTORY "${_output_dir}")
	set(_bdrop_max_size 2048)
	if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
		set(_bdrop_max_size 1024)
	endif()

	WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "UITEXTURE" RESIZE "${_bdrop_max_size}" RDO ENCODING_TARGET texture_encoding TARGET_FOLDER data ALL FILES ${TEXPAGES_BACKDROPS})

	# Excluding pre-generated decal mips
	file(GLOB DECALS_128 LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc*hw-128/*.png")
	if (DECALS_128)
		file(GLOB DECALS_ALL LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc*hw-*/*.png")
		set(DECALS_skipped "${DECALS_ALL}")
		unset(DECALS_ALL)
		foreach(high_qual_decal IN LISTS DECALS_128)
			list(REMOVE_ITEM DECALS_skipped "${high_qual_decal}")
		endforeach()
	endif()

	# The rest of the texture files
	set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/base/texpages")
	file(GLOB_RECURSE ALL_TEXPAGES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/*.png" "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/compression_overrides.txt" "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/*.radar")
	unset(ALL_TEXPAGES_unprocessed)
	list(APPEND ALL_TEXPAGES_unprocessed ${ALL_TEXPAGES})
	list(REMOVE_ITEM ALL_TEXPAGES_unprocessed ${PROCESSED_TEXTURE_FILES})
	list(REMOVE_ITEM ALL_TEXPAGES_unprocessed ${TEXPAGES_BACKDROPS_ALL})
	list(REMOVE_ITEM ALL_TEXPAGES_unprocessed ${DECALS_skipped})
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

	unset(TEXPAGES_BACKDROPS)
	unset(TEXPAGES_BACKDROPS_ALL)
	unset(PROCESSED_TEXTURE_FILES)
	unset(ALL_TEXPAGES)
	unset(TEXTURE_UNPROCESSED_LIST)
	unset(ALL_TEXPAGES_unprocessed)

else()
	if(WZ_ENABLE_BASIS_UNIVERSAL AND WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)
		message(STATUS "WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES is set, textures will not be compressed as part of build. This should only be used for selected CI runs.")
	endif()
endif()

####################################################
# Terrain override mod packing (and image encoding)

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides")
endif()

## Classic overrides

file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/classic.wz")

COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/classic.wz"
	COMPRESSION_LEVEL 0
	PATHS
		"texpages"
		"tileset"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/terrain_overrides/classic"
	BUILD_ALWAYS_TARGET data_terrain_overrides_classic
	IGNORE_GIT
	QUIET
)
set_property(TARGET data_terrain_overrides_classic PROPERTY FOLDER "data")

## High terrain overrides

if (WZ_INCLUDE_TERRAIN_HIGH)

	set(_terrain_overrides_high_base_path "${CMAKE_CURRENT_SOURCE_DIR}/terrain_overrides/high")

	if (WZ_ENABLE_BASIS_UNIVERSAL AND NOT WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)

		set(_build_package "OFF")
		if (WZ_DOWNLOAD_PREBUILT_PACKAGES)
			# Try to download the prebuilt package
			WZ_FETCH_PREBUILT_PACKAGE("${WZ_PREBUILT_TERRAIN_HIGH_DL_URL}" "high.wz" "${WZ_PREBUILT_TERRAIN_HIGH_DL_SHA512}" "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz" _dl_terrain_high_result)
			if (NOT _dl_terrain_high_result)
				# Download failed - WZ_FETCH_PREBUILT_PACKAGE already handles logging a message
				set(_build_package "ON")
			endif()
		else()
			set(_build_package "ON")
		endif()

		if (_build_package)
			# Generate the package

			if(NOT DEFINED BASIS_UNIVERSAL_CLI)
				message(FATAL_ERROR "No basisu tool has been provided - set BASIS_UNIVERSAL_CLI to the path to basisu or disable WZ_ENABLE_BASIS_UNIVERSAL!")
			endif()

			message(STATUS "Configuring to build high.wz package")

			file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz")
			file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high")
			file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high/texpages")
			file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high/tileset")

			set(high_texture_folders "texpages")
			file(GLOB tertiles_dirs LIST_DIRECTORIES true RELATIVE "${_terrain_overrides_high_base_path}" CONFIGURE_DEPENDS "${_terrain_overrides_high_base_path}/texpages/tertilesc*hw-*")
			list(APPEND high_texture_folders ${tertiles_dirs})

			foreach(tex_folder ${high_texture_folders})

				file(GLOB ALL_TEXPAGES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${_terrain_overrides_high_base_path}/${tex_folder}/*.png")
				# Split into _nm, _sm, and everything else (regular texpages)
				file(GLOB TEXPAGES_NM LIST_DIRECTORIES false CONFIGURE_DEPENDS "${_terrain_overrides_high_base_path}/${tex_folder}/*_nm.png")
				file(GLOB TEXPAGES_SM LIST_DIRECTORIES false CONFIGURE_DEPENDS "${_terrain_overrides_high_base_path}/${tex_folder}/*_sm.png")
				unset(TEXPAGES_TEX)
				list(APPEND TEXPAGES_TEX ${ALL_TEXPAGES})
				list(REMOVE_ITEM TEXPAGES_TEX ${TEXPAGES_NM} ${TEXPAGES_SM})

				set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high/${tex_folder}")
				if (tex_folder STREQUAL "texpages")
					set(_terrain_high_max_size 1024)
				else()
					set(_terrain_high_max_size 256) # decals are 256x256
				endif()

				WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "TEXTURE" RESIZE "${_terrain_high_max_size}" UASTC_LEVEL 3 ENCODING_TARGET texture_encoding_high TARGET_FOLDER data ALL FILES ${TEXPAGES_TEX})
				list(APPEND PROCESSED_TEXTURE_FILES ${TEXPAGES_TEX})
				WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "NORMALMAP" RESIZE "${_terrain_high_max_size}" UASTC_LEVEL 3 ENCODING_TARGET texture_encoding_high TARGET_FOLDER data ALL FILES ${TEXPAGES_NM})
				list(APPEND PROCESSED_TEXTURE_FILES ${TEXPAGES_NM})
				WZ_BASIS_ENCODE_TEXTURES(OUTPUT_DIR "${_output_dir}" TYPE "SPECULARMAP" RESIZE "${_terrain_high_max_size}" UASTC_LEVEL 3 ENCODING_TARGET texture_encoding_high TARGET_FOLDER data ALL FILES ${TEXPAGES_SM})
				list(APPEND PROCESSED_TEXTURE_FILES ${TEXPAGES_SM})

			endforeach()

			set(_base_output_dir "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high")
			file(GLOB_RECURSE ALL_TEXPAGES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${_terrain_overrides_high_base_path}/texpages/*.*" "${_terrain_overrides_high_base_path}/tileset/*.*")
			list(APPEND ALL_TEXPAGES_unprocessed ${ALL_TEXPAGES})
			list(REMOVE_ITEM ALL_TEXPAGES_unprocessed ${PROCESSED_TEXTURE_FILES})
			foreach(TEXPAGE_FILE ${ALL_TEXPAGES_unprocessed})
				file(RELATIVE_PATH _output_name "${_terrain_overrides_high_base_path}" "${TEXPAGE_FILE}")
				message(STATUS "Copy unprocessed image file: ${_output_name}")
				add_custom_command(OUTPUT "${_base_output_dir}/${_output_name}"
					COMMAND "${CMAKE_COMMAND}"
					ARGS -E copy_if_different "${TEXPAGE_FILE}" "${_base_output_dir}/${_output_name}"
					WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base/texpages"
					DEPENDS "${TEXPAGE_FILE}"
					VERBATIM
				)
				list(APPEND TEXTURE_UNPROCESSED_LIST "${_base_output_dir}/${_output_name}")
			endforeach()

			add_custom_target(texture_staging_high DEPENDS ${TEXTURE_UNPROCESSED_LIST})
			set_property(TARGET texture_staging_high PROPERTY FOLDER "data")

			unset(PROCESSED_TEXTURE_FILES)
			unset(ALL_TEXPAGES)
			unset(TEXTURE_UNPROCESSED_LIST)
			unset(ALL_TEXPAGES_unprocessed)

			COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz"
				COMPRESSION_LEVEL 0
				PATHS
					"texpages"
					"tileset"
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high"
				BUILD_ALWAYS_TARGET data_terrain_overrides_high
				IGNORE_GIT
				QUIET
			)
			add_dependencies(data_terrain_overrides_high texture_encoding_high texture_staging_high)

		endif(_build_package)
	else()
		if(WZ_ENABLE_BASIS_UNIVERSAL AND WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)
			message(STATUS "WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES is set, textures will not be compressed as part of build. This should only be used for selected CI runs.")
		endif()

		file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz")

		COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz"
			COMPRESSION_LEVEL 0
			PATHS
				"texpages"
				"tileset"
			WORKING_DIRECTORY "${_terrain_overrides_high_base_path}"
			BUILD_ALWAYS_TARGET data_terrain_overrides_high
			IGNORE_GIT
			QUIET
		)
		set_property(TARGET data_terrain_overrides_high PROPERTY FOLDER "data")
	endif()

else()
	message(STATUS "WZ_INCLUDE_TERRAIN_HIGH is OFF. High terrain texture pack will not be part of build.")
endif(WZ_INCLUDE_TERRAIN_HIGH)

#########################
# Compress data archives

COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	COMPRESSION_LEVEL 0
	PATHS
		"GeoIP.dat"
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

add_subdirectory(mods mods_staging)

ADD_CUSTOM_TARGET(data ALL
	DEPENDS data_base data_mp)
if(TARGET data_terrain_overrides_classic)
	add_dependencies(data_base data_terrain_overrides_classic)
endif()
if(TARGET data_terrain_overrides_high)
	add_dependencies(data_base data_terrain_overrides_high)
endif()
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

# Copy mod files to build directory to support IDE build+run (from the build dir) scenarios
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/mods")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mods")
endif()
add_dependencies(data data_mods)
foreach(mod_file ${wz2100_mods_FILES})
	file(RELATIVE_PATH _mods_file_relative_path "${CMAKE_CURRENT_BINARY_DIR}/mods_staging" "${mod_file}")
	get_filename_component(_mods_file_subdir_path "${_mods_file_relative_path}" DIRECTORY)
	get_filename_component(_mods_file_name "${_mods_file_relative_path}" NAME)
    add_custom_command(
		TARGET data
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${mod_file}" "${CMAKE_CURRENT_BINARY_DIR}/mods/${_mods_file_subdir_path}/${_mods_file_name}"
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

set(DATA_TERRAIN_OVERRIDES_FILES
	"${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/classic.wz"
)
if(TARGET data_terrain_overrides_high OR EXISTS "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz")
	list(APPEND DATA_TERRAIN_OVERRIDES_FILES "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/high.wz")
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "Emscripten")

	install(FILES ${DATA_FILES}
		DESTINATION "${WZ_DATADIR}"
		COMPONENT Data
	)
	install(FILES ${DATA_TERRAIN_OVERRIDES_FILES}
		DESTINATION "${WZ_DATADIR}/terrain_overrides"
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

	foreach(_mod_file ${wz2100_mods_FILES})
		file(RELATIVE_PATH _mods_file_relative_path "${CMAKE_CURRENT_BINARY_DIR}/mods_staging" "${_mod_file}")
		get_filename_component(_mods_file_subdir_path "${_mods_file_relative_path}" DIRECTORY)
		install(FILES ${_mod_file}
			DESTINATION "${WZ_DATADIR}/mods/${_mods_file_subdir_path}"
			COMPONENT Data
		)
	endforeach()

else()
	# Emscripten:

	if (NOT DEFINED EMSCRIPTEN_ROOT_PATH OR NOT EMSCRIPTEN_ROOT_PATH)
		message(WARNING "Invalid EMSCRIPTEN_ROOT_PATH? (=${EMSCRIPTEN_ROOT_PATH})")
	endif()
	find_file(EMSCRIPTEN_FILE_PACKAGER_PY NAMES "file_packager.py" PATHS "${EMSCRIPTEN_ROOT_PATH}/tools" NO_CMAKE_FIND_ROOT_PATH)
	if (NOT EMSCRIPTEN_FILE_PACKAGER_PY)
		message(FATAL_ERROR "Unable to find Emscripten file_packager.py")
	endif()
	find_package (Python3 COMPONENTS Interpreter REQUIRED)

	# Bundle the classic terrain into a separate package
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/empackaged/terrain_overrides")
	add_custom_target(
		data_terrain_overrides_classic_packaging ALL
		${Python3_EXECUTABLE} "${EMSCRIPTEN_FILE_PACKAGER_PY}" warzone2100-terrain-classic.data --preload "${CMAKE_CURRENT_BINARY_DIR}/terrain_overrides/classic.wz@/data/terrain_overrides/classic/" --js-output=warzone2100-terrain-classic.js --use-preload-cache --indexedDB-name=EM_PRELOAD_TERRAIN_CLASSIC_CACHE --no-node
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/empackaged/terrain_overrides"
		DEPENDS data_terrain_overrides_classic
		VERBATIM
	)
	list(APPEND DATA_ADDITIONAL_EMPACKAGE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/empackaged/terrain_overrides")

	# Bundle the original music into a separate package
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/music_staging")
	file(GLOB DATA_ORIG_MUSIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/music/*.opus" "${CMAKE_CURRENT_SOURCE_DIR}/music/albums/original_soundtrack/*.*")
	foreach(_music_file ${DATA_ORIG_MUSIC_FILES})
		file(RELATIVE_PATH _music_file_relative_path "${CMAKE_CURRENT_SOURCE_DIR}/music" "${_music_file}")
		get_filename_component(_music_file_subdir_path "${_music_file_relative_path}" DIRECTORY)
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/music_staging/${_music_file_subdir_path}")
		configure_file("${_music_file}" "${CMAKE_CURRENT_BINARY_DIR}/music_staging/${_music_file_subdir_path}" COPYONLY)
	endforeach()
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/empackaged/music")
	add_custom_target(
		data_music_empackage ALL
		${Python3_EXECUTABLE} "${EMSCRIPTEN_FILE_PACKAGER_PY}" warzone2100-music.data --preload "${CMAKE_CURRENT_BINARY_DIR}/music_staging@/data/music/" --js-output=warzone2100-music.js --use-preload-cache --indexedDB-name=EM_PRELOAD_MUSIC_CACHE --no-node
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/empackaged/music"
		VERBATIM
	)
	list(APPEND DATA_ADDITIONAL_EMPACKAGE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/empackaged/music")

	set(DATA_ADDITIONAL_EMPACKAGE_DIRS ${DATA_ADDITIONAL_EMPACKAGE_DIRS} PARENT_SCOPE)
	set(DATA_ADDITIONAL_EMPACKAGE_BASEDIR "${CMAKE_CURRENT_BINARY_DIR}/empackaged" PARENT_SCOPE)


endif()

set(DATA_FILES ${DATA_FILES} PARENT_SCOPE)
set(DATA_TERRAIN_OVERRIDES_FILES ${DATA_TERRAIN_OVERRIDES_FILES} PARENT_SCOPE)
set(DATA_FONTS ${wz2100_fonts_FILES} PARENT_SCOPE)
set(DATA_MUSIC_FILES ${DATA_MUSIC_FILES} PARENT_SCOPE)
set(DATA_MUSIC_BASE_SOURCEDIR "${CMAKE_CURRENT_SOURCE_DIR}/music" PARENT_SCOPE)
set(DATA_MODS_FILES ${wz2100_mods_FILES} PARENT_SCOPE)
set(DATA_MODS_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/mods_staging" PARENT_SCOPE)
