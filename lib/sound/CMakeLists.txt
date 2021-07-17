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
target_link_libraries(sound PRIVATE framework OpenAL::OpenAL ${OGGVORBIS_LIBRARIES})
