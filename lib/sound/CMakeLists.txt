file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

set(_openal_config_required)
set(_openal_config_quiet QUIET)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# CONFIG mode should be REQUIRED (to avoid the deprecated OpenAL.framework)
	set(_openal_config_required REQUIRED)
	set(_openal_config_quiet)
endif()
find_package(OpenAL CONFIG ${_openal_config_quiet} ${_openal_config_required}) # explicitly try via CONFIG mode first
if (NOT TARGET OpenAL::OpenAL)
	find_package(OpenAL MODULE REQUIRED)
	# Since FindOpenAL.cmake may not define an imported target, do it here
	if (NOT TARGET OpenAL::OpenAL)
		add_library(OpenAL::OpenAL UNKNOWN IMPORTED)
		set_target_properties(OpenAL::OpenAL PROPERTIES
		  INTERFACE_INCLUDE_DIRECTORIES ${OPENAL_INCLUDE_DIR}
		  IMPORTED_LOCATION ${OPENAL_LIBRARY}
		)
	endif()
endif()
find_package(OggVorbis REQUIRED)
find_package(Ogg REQUIRED)
find_package(Opus REQUIRED)

INCLUDE(CMakePushCheckState)
INCLUDE(CheckIncludeFileCXX)
get_target_property(_openal_includes OpenAL::OpenAL INTERFACE_INCLUDE_DIRECTORIES)
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES ${_openal_includes})
message(STATUS "OpenAL INTERFACE_INCLUDE_DIRECTORIES = \"${_openal_includes}\"")
CHECK_INCLUDE_FILE_CXX("AL/alext.h" HAVE_OPENAL_ALEXT_H)
cmake_pop_check_state()

add_library(sound STATIC ${HEADERS} ${SRC})
set_property(TARGET sound PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sound)
target_compile_definitions(sound PRIVATE "-DAL_LIBTYPE_STATIC")
if (HAVE_OPENAL_ALEXT_H)
	target_compile_definitions(sound PRIVATE "-DHAVE_OPENAL_ALEXT_H")
endif()
target_include_directories(sound PRIVATE "${OGGVORBIS_INCLUDE_DIR}")
target_link_libraries(sound PRIVATE framework nlohmann_json OpenAL::OpenAL ${OGGVORBIS_LIBRARIES})
target_link_libraries(sound PRIVATE Ogg::ogg Opus::opus)
target_link_libraries(sound PUBLIC optional-lite)

# Opusfile
find_package(Opusfile QUIET)
if (NOT WZ_FORCE_MINIMAL_OPUSFILE AND Opusfile_FOUND)
	# Link the found Opusfile
	message(STATUS "Found Opusfile library: ${OPUSFILE_LIBRARY}")
	target_link_libraries(sound PRIVATE Opus::opusfile)
else()
	# Build and link the in-repo Opusfile submodule
	message(STATUS "Building Opusfile from source")
	add_library(opusfile STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/include/opusfile.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/src/info.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/src/internal.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/src/internal.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/src/opusfile.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/src/stream.c"
	)
	# opusfile.h includes "opus_multistream.h" instead of "opus/opus_multistream.h"
	# so add "opus" subdirectory to fix that
	set(OPUS_MULTISTREAM_DIR "${OPUS_INCLUDE_DIR}/opus")
	target_include_directories(opusfile PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opusfile/include" "${OPUS_MULTISTREAM_DIR}")
	target_link_libraries(opusfile PRIVATE Ogg::ogg Opus::opus)
	include(CMakePushCheckState)
	include(CheckSymbolExists)
	cmake_push_check_state(RESET)
	include(CheckLibraryExists)
	check_library_exists(m lrintf "" OP_HAVE_LIBM)
	if(OP_HAVE_LIBM)
		target_link_libraries(opusfile PRIVATE ${OGG_LIBRARY} ${OPUS_LIBRARY})
		list(APPEND CMAKE_REQUIRED_LIBRARIES "m")
	endif()
	check_symbol_exists(lrintf "math.h" OP_HAVE_LRINTF)
	cmake_pop_check_state()
	if(OP_HAVE_LRINTF)
		target_compile_definitions(opusfile PRIVATE OP_HAVE_LRINTF)
	endif()
	set_property(TARGET opusfile PROPERTY FOLDER "3rdparty")
	target_link_libraries(sound PRIVATE opusfile)
endif()
