cmake_minimum_required(VERSION 3.5)
project(data)

find_package(ZIP REQUIRED)

COMPRESS_ZIP("${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	COMPRESSION_LEVEL 0
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base"
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
	QUIET
)

COMPRESS_ZIP("${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
	COMPRESSION_LEVEL 0
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mp"
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
	QUIET
)

ADD_CUSTOM_TARGET(data ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/base.wz" "${CMAKE_CURRENT_BINARY_DIR}/mp.wz")

set(DATA_FILES
	"${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	"${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
)
install(FILES ${DATA_FILES}
	DESTINATION "data"
	COMPONENT Core
)

set(DATA_MUSIC_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/music/menu.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track1.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track2.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track3.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/music.wpl"
)
install(FILES ${DATA_MUSIC_FILES}
	DESTINATION "data/music"
	COMPONENT Core
)

set(DATA_FILES ${DATA_FILES} PARENT_SCOPE)
set(DATA_MUSIC_FILES ${DATA_MUSIC_FILES} PARENT_SCOPE)
