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
	)

ADD_CUSTOM_TARGET(data_base ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/base.wz")

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
	)

ADD_CUSTOM_TARGET(data_mp ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mp.wz")

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/base.wz"
	"${CMAKE_CURRENT_BINARY_DIR}/mp.wz"
	DESTINATION "data"
	COMPONENT Core
)
install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/music/menu.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track1.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track2.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track3.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/music.wpl"
	DESTINATION "data/music"
	COMPONENT Core
)
