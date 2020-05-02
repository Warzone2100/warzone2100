cmake_minimum_required (VERSION 3.5)

add_subdirectory(micro-ecc)
set_property(TARGET microecc PROPERTY FOLDER "3rdparty")

add_subdirectory(fonts)

SET(UTF8PROC_INSTALL OFF CACHE BOOL "Enable installation of utf8proc" FORCE)
add_subdirectory(utf8proc)
set_property(TARGET utf8proc PROPERTY FOLDER "3rdparty")

add_subdirectory(launchinfo)
set_property(TARGET launchinfo PROPERTY FOLDER "3rdparty")

SET(RE2_BUILD_TESTING OFF CACHE BOOL "enable testing for RE2" FORCE)
add_subdirectory(re2 EXCLUDE_FROM_ALL)
target_include_directories(re2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/re2>
)
set_property(TARGET re2 PROPERTY FOLDER "3rdparty")
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS NO)	# -Wmissing-field-initializers
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-missing-field-initializers")

add_subdirectory(EmbeddedJSONSignature)
set_property(TARGET EmbeddedJSONSignature PROPERTY FOLDER "3rdparty")

# SHA2-library

set(SHA2_HEADERS
	sha/brg_endian.h
	sha/brg_types.h
	sha/sha2.h
)

set(SHA2_SOURCES
	sha/sha2.c
)

add_library(sha2 STATIC ${SHA2_HEADERS} ${SHA2_SOURCES})
set_property(TARGET sha2 PROPERTY FOLDER "3rdparty")
target_include_directories(sha2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
