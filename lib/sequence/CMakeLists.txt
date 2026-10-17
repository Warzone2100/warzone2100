set(HEADERS
	ogg_decoder.h
	sequence.h
	video_decoder.h
	video_edits.h
	video_provider.h
	video_timeline.h
)
set(SRC
	ogg_decoder.cpp
	sequence.cpp
	video_decoder.cpp
	video_edits.cpp
	video_provider.cpp
	video_timeline.cpp
)

if(WZ_ENABLE_WEBM)
	list(APPEND HEADERS webm_decoder.h webm_video_codec.h vp9_bitstream.h)
	list(APPEND SRC webm_decoder.cpp webm_video_codec.cpp video_codec_vpx.cpp vp9_bitstream.cpp)
endif()

find_package(OggVorbis REQUIRED)
find_package(Theora REQUIRED)

add_library(sequence STATIC ${HEADERS} ${SRC})
set_property(TARGET sequence PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sequence)
target_include_directories(sequence PRIVATE ivis-opengl "${OGGVORBIS_INCLUDE_DIR}")
target_link_libraries(sequence PRIVATE framework sound ${OGGVORBIS_LIBRARIES} theora::dec)

if(WZ_ENABLE_WEBM)
	# libvpx: vcpkg provides a CMake config package ("unofficial-libvpx")
	# Linux system packages (libvpx-dev or equivalent) provide vpx.pc for pkg-config
	find_package(unofficial-libvpx CONFIG QUIET)
	if(TARGET unofficial::libvpx::libvpx)
		message(STATUS "Found libvpx via CMake config package")
		set(_wz_vpx_lib unofficial::libvpx::libvpx)
	else()
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(VPX REQUIRED IMPORTED_TARGET vpx)
		message(STATUS "Found libvpx via pkg-config: ${VPX_VERSION}")
		set(_wz_vpx_lib PkgConfig::VPX)
	endif()
	target_compile_definitions(sequence PRIVATE "WZ_ENABLE_WEBM")
	target_link_libraries(sequence PRIVATE ${_wz_vpx_lib} mkvparser)

	if(WZ_ENABLE_VIDEO_BACKEND_MF AND CMAKE_SYSTEM_NAME MATCHES "Windows")
		# mfplat.dll is loaded at runtime instead, so the game still starts without Media Foundation
		target_sources(sequence PRIVATE "video_codec_mf_win.cpp")
		target_compile_definitions(sequence PRIVATE "WZ_ENABLE_VIDEO_BACKEND_MF")
		target_link_libraries(sequence PRIVATE mfuuid strmiids ole32 oleaut32)
	endif()
endif()
