cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	OPTION(WZ_INCLUDE_VIDEOS "Download & include videos in the application package" OFF)
endif()

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
	IGNORE_GIT
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
	IGNORE_GIT
	QUIET
)

if(WZ_INCLUDE_VIDEOS)
	set(_download_script_path "${PROJECT_SOURCE_DIR}/cmake/FetchURLRetry.cmake")
	set(_sequences_dl_filename "sequences.wz")
	set(_sequences_dl_url "https://downloads.sourceforge.net/project/warzone2100/warzone2100/Videos/standard-quality-en/sequences.wz")
	set(_sequences_dl_sha256 "142ae905be288cca33357a49f42b884c190e828fc0b1b1773ded5dff774f41a3")
	set(_sequences_output_dir "${CMAKE_CURRENT_BINARY_DIR}")
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
	if(NOT _exstatus EQUAL 0)
		message(FATAL_ERROR "Failed to download sequences.wz (campaign videos)")
	endif()
endif()

ADD_CUSTOM_TARGET(data ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/base.wz" "${CMAKE_CURRENT_BINARY_DIR}/mp.wz")

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

file(GLOB DATA_MUSIC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/music/*.ogg" "${CMAKE_CURRENT_SOURCE_DIR}/music/music.wpl")
install(FILES ${DATA_MUSIC_FILES}
	DESTINATION "${WZ_DATADIR}/music"
	COMPONENT Data
)

set(DATA_FILES ${DATA_FILES} PARENT_SCOPE)
set(DATA_MUSIC_FILES ${DATA_MUSIC_FILES} PARENT_SCOPE)
