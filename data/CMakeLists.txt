project(data)


add_custom_target(data_base ALL COMMAND
    zip -r0 ${CMAKE_CURRENT_BINARY_DIR}/base.wz
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
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base")

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
	DESTINATION ".")
install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/music/menu.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track1.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track2.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/track3.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/music/music.wpl"
	DESTINATION "music")
