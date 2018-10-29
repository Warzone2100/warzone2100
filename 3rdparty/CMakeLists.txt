cmake_minimum_required (VERSION 3.5)

add_subdirectory(micro-ecc)
add_subdirectory(fonts)
add_subdirectory(utf8proc)

# SHA2-library

set(SHA2_HEADERS
	sha2/brg_endian.h
	sha2/brg_types.h
	sha2/sha2.h
)

set(SHA2_SOURCES
	sha2/sha2.c
)

add_library(sha2 STATIC ${SHA2_HEADERS} ${SHA2_SOURCES})
target_include_directories(sha2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
