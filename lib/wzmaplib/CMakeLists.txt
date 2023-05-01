###################
# wzmaplib version

set(WZMAPLIB_VERSION_MAJOR 1)
set(WZMAPLIB_VERSION_MINOR 1)
set(WZMAPLIB_VERSION_REV 0)

###################
# Determine minimum CMake version

set(_cmake_min_ver_supported 3.5)
if (TARGET nlohmann_json AND TARGET optional-lite AND TARGET quickjs)
	# This should happen if being included in the larger WZ build scripts, thus we don't need FetchContent and can support earlier CMake
	message(DEBUG "All required dependencies are already available - no need to use FetchContent")
else()
	message(STATUS "wzmaplib: One or more dependencies must be fetched")
	set(_cmake_min_ver_supported 3.11) # CMake 3.11 is the minimum version for FetchContent
	set(_will_need_fetch_content TRUE)
endif()

##############################
##############################
# wzmaplib project

cmake_minimum_required(VERSION ${_cmake_min_ver_supported}...3.18)
project (wzmaplib CXX)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

file(GLOB PUBLIC_HEADERS "include/wzmaplib/*.h")
file(GLOB PRIVATE_HEADERS "src/*.h")
file(GLOB SRC "src/*.cpp")

set(WZMAPLIB_VERSION_STRING "${WZMAPLIB_VERSION_MAJOR}.${WZMAPLIB_VERSION_MINOR}.${WZMAPLIB_VERSION_REV}")
configure_file("src/map_config_internal.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated-include/wzmaplib_internal/map_config_internal.h" @ONLY)

###################
# wzmaplib library

add_library(wzmaplib STATIC ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SRC})
set_property(TARGET wzmaplib PROPERTY FOLDER "lib")
set_target_properties(wzmaplib PROPERTIES VERSION "${WZMAPLIB_VERSION_STRING}")

set_target_properties(wzmaplib
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)

# Support being included directly in WZ build (with its custom target configuration)
include(WZTargetConfiguration OPTIONAL RESULT_VARIABLE _wztargetconfiguration_module_path)
if(_wztargetconfiguration_module_path)
	WZ_TARGET_CONFIGURATION(wzmaplib)
else()
	message(VERBOSE "Unable to find WZTargetConfiguration - continuing without")
endif()

if(MSVC)
	target_compile_definitions(wzmaplib PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
target_include_directories(wzmaplib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated-include/") # for map_config_internal.h
target_include_directories(wzmaplib PUBLIC "include")

########################
# wzmaplib dependencies

if(_will_need_fetch_content)
	include(FetchContent)
endif()

if(NOT TARGET nlohmann_json)
	message(STATUS "wzmaplib: Fetching nlohmann/json")
	set(JSON_ImplicitConversions OFF CACHE BOOL "Enable implicit conversions." FORCE)
	set(JSON_SystemInclude ON CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)
	FetchContent_Declare(
		nlohmannjson
		GIT_REPOSITORY https://github.com/nlohmann/json.git
		GIT_TAG        v3.11.2
		GIT_SHALLOW    TRUE
	)
	FetchContent_GetProperties(nlohmannjson)
	if(NOT nlohmannjson_POPULATED)
		FetchContent_Populate(nlohmannjson)
	endif()
	add_subdirectory("${nlohmannjson_SOURCE_DIR}" "${nlohmannjson_BINARY_DIR}" EXCLUDE_FROM_ALL)
	# Temporary workaround until we use the new NLOHMANN_JSON_NAMESPACE (etc) macros
	target_compile_definitions(nlohmann_json INTERFACE "NLOHMANN_JSON_NAMESPACE=nlohmann;NLOHMANN_JSON_NAMESPACE_BEGIN=namespace nlohmann {;NLOHMANN_JSON_NAMESPACE_END=}")
	# Manually copy json_fwd.hpp to the single_include directory (from the include directory)
	file(COPY "${nlohmannjson_SOURCE_DIR}/include/nlohmann/json_fwd.hpp" DESTINATION "${nlohmannjson_SOURCE_DIR}/single_include/nlohmann")
endif()
target_link_libraries(wzmaplib PRIVATE nlohmann_json)
if(NOT TARGET optional-lite)
	message(STATUS "wzmaplib: Fetching martinmoene/optional-lite")
	FetchContent_Declare(
		optionallite
		GIT_REPOSITORY https://github.com/martinmoene/optional-lite.git
		GIT_TAG        v3.5.0
		GIT_SHALLOW    TRUE
	)
	FetchContent_GetProperties(optionallite)
	if(NOT optionallite_POPULATED)
		FetchContent_Populate(optionallite)
	endif()
	add_subdirectory("${optionallite_SOURCE_DIR}" "${optionallite_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()
target_link_libraries(wzmaplib PUBLIC optional-lite)
if(NOT TARGET quickjs)
	message(STATUS "wzmaplib: Fetching Warzone2100/quickjs-wz")
	FetchContent_Declare(
	quickjs_wz
		GIT_REPOSITORY  https://github.com/Warzone2100/quickjs-wz.git
		GIT_TAG         origin/main
		GIT_REMOTE_NAME origin
		GIT_SHALLOW     TRUE
	)
	FetchContent_GetProperties(quickjs_wz)
	if(NOT quickjs_wz_POPULATED)
		FetchContent_Populate(quickjs_wz)
	endif()
	add_subdirectory("${quickjs_wz_SOURCE_DIR}" "${quickjs_wz_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()
target_link_libraries(wzmaplib PRIVATE quickjs)

############
# [Plugins]

##################################
# ZipIOProvider (requires libzip)

find_package(libzip QUIET)
if (libzip_FOUND)
	# Test linking libzip
	include(CheckCXXSourceCompiles)
	include(CMakePushCheckState)
	set(_test_libzip_source
		"#include <zip.h>\n \
		void foo() { /* do nothing */ }\n \
		int main() {\n \
			const char* libzip_ver_string = zip_libzip_version();\n \
			if (!libzip_ver_string) { return 1; }\n \
			return 0;\n \
		}"
	)
	cmake_push_check_state(RESET)
	set(CMAKE_REQUIRED_LIBRARIES "libzip::zip")
	# libzip exported CMake config does not currently include all required support libraries - see: https://github.com/nih-at/libzip/issues/205
	# So manually find and link them
	find_package (ZLIB REQUIRED)
	list(APPEND CMAKE_REQUIRED_LIBRARIES "ZLIB::ZLIB")
	find_package (BZip2)
	if (BZip2_FOUND)
		list(APPEND CMAKE_REQUIRED_LIBRARIES "BZip2::BZip2")
	endif()
	check_cxx_source_compiles("${_test_libzip_source}" libzip_LINK_TEST)
	cmake_pop_check_state()
	if(libzip_LINK_TEST)
		message( STATUS "Found libzip... (link test successful)" )
	else()
		message( STATUS "Found libzip, but link test NOT successful" )
	endif()
endif()
if (libzip_FOUND AND libzip_LINK_TEST)
	file(GLOB ZIPIOPROVIDER_HEADERS "plugins/ZipIOProvider/include/ZipIOProvider.h")
	file(GLOB ZIPIOPROVIDER_SRC "plugins/ZipIOProvider/src/ZipIOProvider.cpp")

	add_library(ZipIOProvider STATIC ${ZIPIOPROVIDER_HEADERS} ${ZIPIOPROVIDER_SRC})
	set_property(TARGET ZipIOProvider PROPERTY FOLDER "lib")
	set_target_properties(ZipIOProvider
		PROPERTIES
			CXX_STANDARD 11
			CXX_STANDARD_REQUIRED YES
			CXX_EXTENSIONS NO
	)
	if(_wztargetconfiguration_module_path)
		WZ_TARGET_CONFIGURATION(ZipIOProvider)
	else()
		message(STATUS "Unable to find WZTargetConfiguration - continuing without")
	endif()
	if(MSVC)
		target_compile_definitions(ZipIOProvider PRIVATE "_CRT_SECURE_NO_WARNINGS")
	endif()
	target_include_directories(ZipIOProvider PUBLIC "plugins/ZipIOProvider/include")
	target_link_libraries(ZipIOProvider PUBLIC wzmaplib)

	target_link_libraries(ZipIOProvider PRIVATE libzip::zip)
	# libzip exported CMake config does not currently include all required support libraries - see: https://github.com/nih-at/libzip/issues/205
	# So manually find and link them
	find_package (ZLIB REQUIRED)
	target_link_libraries(ZipIOProvider PRIVATE ZLIB::ZLIB)
	find_package (BZip2)
	if (BZip2_FOUND)
		target_link_libraries(ZipIOProvider PRIVATE BZip2::BZip2)
	endif()
else()
	message(STATUS "Could NOT find libzip - ZipIOProvider target will not be available")
endif()
