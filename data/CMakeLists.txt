project(data)

include(ExternalProject)
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	ExternalProject_Add(
	  nvidia-texture-tools
	  GIT_REPOSITORY "https://github.com/vlj/nvidia-texture-tools.git"
	  GIT_TAG "master"
	  SOURCE_DIR "${CMAKE_BINARY_DIR}/nvtt"
	  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/nvtt/install
	)
	set (nvcompress "${CMAKE_BINARY_DIR}/nvtt/install/bin/nvcompress.exe")
else()
	find_program (nvcompress "nvcompress")
endif()

file(GLOB TEXTURES_INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/bdrops/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/images/frontend/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/images/intfac/*.png"
)

file(GLOB TEXTURE_3D
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc1hw-128/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc1hw-16/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc1hw-32/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc1hw-64/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc2hw-128/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc2hw-16/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc2hw-32/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc2hw-64/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc3hw-128/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc3hw-16/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc3hw-32/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/base/texpages/tertilesc3hw-64/*.png"
)

set(DDS_FILES "")

#Note: only diff here is between build is nvidia-texture-tools dependency
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	foreach(TEXTURE ${TEXTURES_INTERFACE})
		get_filename_component(PNG_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(PNG_FILE ${TEXTURE} NAME_WE)
		add_custom_command(OUTPUT "${PNG_FILE_PATH}/${PNG_FILE}.dds"
			COMMAND "${nvcompress}" "-color" "-alpha" "-rgb" "-nomips" "${TEXTURE}"# "${CMAKE_CURRENT_BINARY_DIR}/data/base/texpages/bdrops/backdrop0.dds"
			DEPENDS "${TEXTURE}" nvidia-texture-tools
			)
		list(APPEND DDS_FILES "${PNG_FILE_PATH}/${PNG_FILE}.dds;")
	endforeach()

	foreach(TEXTURE ${TEXTURE_3D})
		get_filename_component(PNG_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(PNG_FILE ${TEXTURE} NAME_WE)
		add_custom_command(OUTPUT "${PNG_FILE_PATH}/${PNG_FILE}.dds"
			COMMAND "${nvcompress}" "-color" "-alpha" "-rgb" "${TEXTURE}"# "${CMAKE_CURRENT_BINARY_DIR}/data/base/texpages/bdrops/backdrop0.dds"
			DEPENDS "${TEXTURE}" nvidia-texture-tools
			)
		list(APPEND DDS_FILES "${PNG_FILE_PATH}/${PNG_FILE}.dds;")
	endforeach()
else()
	foreach(TEXTURE ${TEXTURES_INTERFACE})
		get_filename_component(PNG_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(PNG_FILE ${TEXTURE} NAME_WE)
		add_custom_command(OUTPUT "${PNG_FILE_PATH}/${PNG_FILE}.dds"
			COMMAND "${nvcompress}" "-color" "-alpha" "-rgb" "-nomips" "${TEXTURE}"# "${CMAKE_CURRENT_BINARY_DIR}/data/base/texpages/bdrops/backdrop0.dds"
			DEPENDS "${TEXTURE}"
			)
		list(APPEND DDS_FILES "${PNG_FILE_PATH}/${PNG_FILE}.dds;")
	endforeach()

	foreach(TEXTURE ${TEXTURE_3D})
		get_filename_component(PNG_FILE_PATH ${TEXTURE} DIRECTORY)
		get_filename_component(PNG_FILE ${TEXTURE} NAME_WE)
		add_custom_command(OUTPUT "${PNG_FILE_PATH}/${PNG_FILE}.dds"
			COMMAND "${nvcompress}" "-color" "-alpha" "-rgb" "${TEXTURE}"# "${CMAKE_CURRENT_BINARY_DIR}/data/base/texpages/bdrops/backdrop0.dds"
			DEPENDS "${TEXTURE}"
			)
		list(APPEND DDS_FILES "${PNG_FILE_PATH}/${PNG_FILE}.dds;")
	endforeach()
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	install(DIRECTORY
		"base/audio"
		"base/campaigns"
		"base/components"
		"base/effects"
		"base/features"
		"base/images"
		"base/messages"
		"base/misc"
		"base/fonts"
		"base/script"
		"base/sequenceaudio"
		"base/shaders"
		"base/stats"
		"base/structs"
		"base/texpages"
		"base/tileset"
		"base/wrf"
		COMPONENT Data
		DESTINATION "data/base/")
	install(DIRECTORY
		"mp/anims"
		"mp/challenges"
		"mp/tests"
		"mp/components"
		"mp/effects"
		"mp/messages"
		"mp/multiplay"
		"mp/stats"
		"mp/structs"
		"mp/wrf"
		COMPONENT Data
		DESTINATION "data/mp/")
	install(FILES
		"base/palette.txt"
		"base/ruleset.json"
		"base/gamedesc.lev"
		COMPONENT Data
		DESTINATION "data/base")
	install(FILES
		"mp/addon.lev"
		"mp/ruleset.json"
		COMPONENT Data
		DESTINATION "data/mp")
	add_custom_target(data_base ALL
		DEPENDS ${DDS_FILES})
else()
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/base.wz
		COMMAND zip -r0 ${CMAKE_CURRENT_BINARY_DIR}/base.wz
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
		DEPENDS ${DDS_FILES})
	
	add_custom_target(data_mp ALL COMMAND
		zip -r0 ${CMAKE_CURRENT_BINARY_DIR}/mp.wz
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
	)
	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/base.wz"
		"${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
		COMPONENT Data
		DESTINATION ".")
	add_custom_target(data_base ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/base.wz")
endif()

install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/music/menu.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track1.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track2.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track3.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/music.wpl"
	COMPONENT Data
	DESTINATION "music")
