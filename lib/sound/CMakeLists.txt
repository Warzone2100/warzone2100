cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

find_package(OpenAL REQUIRED)
find_package(OggVorbis REQUIRED)

add_library(sound ${HEADERS} ${SRC})
target_compile_definitions(sound PRIVATE "-DAL_LIBTYPE_STATIC")
target_include_directories(sound PRIVATE "${OPENAL_INCLUDE_DIR}" "${OGGVORBIS_INCLUDE_DIR}")
target_link_libraries(sound PRIVATE framework ${OPENAL_LIBRARY} ${OGGVORBIS_LIBRARIES})
