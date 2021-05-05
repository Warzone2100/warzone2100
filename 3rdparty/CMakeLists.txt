cmake_minimum_required (VERSION 3.5)

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

add_subdirectory(quickjs EXCLUDE_FROM_ALL)
set_property(TARGET quickjs PROPERTY FOLDER "3rdparty")
