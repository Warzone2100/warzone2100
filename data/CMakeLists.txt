project(data)


add_custom_target(data_base COMMAND
    ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_CURRENT_BINARY_DIR}/base.wz" --format=zip
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

add_custom_target(data_mp COMMAND
    ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_CURRENT_BINARY_DIR}/mp.wz" --format=zip
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
