cmake_minimum_required (VERSION 3.5)

SET(UTF8PROC_INSTALL OFF CACHE BOOL "Enable installation of utf8proc" FORCE)
add_subdirectory(utf8proc)
set_property(TARGET utf8proc PROPERTY FOLDER "3rdparty")

add_subdirectory(launchinfo)
set_property(TARGET launchinfo PROPERTY FOLDER "3rdparty")

add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "3rdparty")

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

if(ENABLE_DISCORD)
	add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
	set_property(TARGET discord-rpc PROPERTY FOLDER "3rdparty")
endif()

find_package(SQLite3 3.14 REQUIRED)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)
set(SQLITE_ENABLE_COLUMN_METADATA OFF CACHE BOOL "Enable Column::getColumnOriginName(). Require support from sqlite3 library." FORCE)
set(SQLITE_HAS_CODEC OFF CACHE BOOL "Enable database encryption API. Not available in the public release of SQLite." FORCE)
if(SQLite3_VERSION VERSION_LESS 3.19)
	set(SQLITE_USE_LEGACY_STRUCT ON CACHE BOOL "Fallback to forward declaration of legacy struct sqlite3_value (pre SQLite 3.19)" FORCE)
endif()
set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "Add the internal SQLite3 source to the project." FORCE)
set(SQLITECPP_INCLUDE_SCRIPT OFF CACHE BOOL "Include config & script files." FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Run cpplint.py tool for Google C++ StyleGuide." FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "Run cppcheck C++ static analysis tool." FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "Build examples." FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "Build and run tests." FORCE)
add_subdirectory(SQLiteCpp EXCLUDE_FROM_ALL)
set_property(TARGET SQLiteCpp PROPERTY FOLDER "3rdparty")

# glad library

set(GLAD_HEADERS
	glad/include/glad/glad.h
	glad/include/KHR/khrplatform.h
)

set(GLAD_SOURCES
	glad/src/glad.c
)

add_library(glad STATIC ${GLAD_HEADERS} ${GLAD_SOURCES})
set_property(TARGET glad PROPERTY FOLDER "3rdparty")
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

set(JSON_ImplicitConversions OFF CACHE BOOL "Enable implicit conversions." FORCE)
set(JSON_SystemInclude ON CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)
add_subdirectory(json EXCLUDE_FROM_ALL)

add_subdirectory(optional-lite EXCLUDE_FROM_ALL)

add_subdirectory(quickjs-wz EXCLUDE_FROM_ALL)
set_property(TARGET quickjs PROPERTY FOLDER "3rdparty")

# basis-universal transcoder

add_library(basis_transcoder STATIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.h" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/zstd/zstddeclib.c")
set_property(TARGET basis_transcoder PROPERTY FOLDER "3rdparty")
target_include_directories(basis_transcoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder")

include(CheckCompilerFlagsOutput)
if(NOT MSVC)
	set(_supported_basistranscoder_cxx_compiler_flags "")

	# -fno-strict-aliasing		(GCC, Clang)
	check_compiler_flags_output("-Werror -fno-strict-aliasing -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-fno-strict-aliasing" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

	if (NOT _supported_basistranscoder_cxx_compiler_flags STREQUAL "")
		string(REPLACE " " ";" _supported_basistranscoder_cxx_compiler_flags "${_supported_basistranscoder_cxx_compiler_flags}")
		target_compile_options(basis_transcoder PRIVATE ${_supported_basistranscoder_cxx_compiler_flags})
	endif()
endif()

## TODO: Must also set BASISD_IS_BIG_ENDIAN if the target platform is big-endian!!
# if (# SOME DETECTION FOR TARGET PLATFORM BIG-ENDIAN #)
# 	target_compile_definitions(basis_transcoder PRIVATE "-DBASISD_IS_BIG_ENDIAN=1")
# endif()

# Disable certain transcoder formats
target_compile_definitions(basis_transcoder PRIVATE "-DBASISD_SUPPORT_ATC=0" "-DBASISD_SUPPORT_PVRTC1=0" "-DBASISD_SUPPORT_PVRTC2=0")
