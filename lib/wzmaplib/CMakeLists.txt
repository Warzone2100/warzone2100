###################
# wzmaplib version

set(WZMAPLIB_VERSION_MAJOR 1)
set(WZMAPLIB_VERSION_MINOR 4)
set(WZMAPLIB_VERSION_REV 5)

##############################
##############################
# wzmaplib project

cmake_minimum_required(VERSION 3.16...3.31)
project (wzmaplib CXX)

OPTION(WZMAPLIB_ENABLE_ZIPIOPROVIDER "wzmaplib: Provide ZipIOProvider (which requires libzip)" ON)

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
		CXX_STANDARD 17
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

include(FetchContent)

function(WZMAPLIB_ADD_3RDPARTY_DIR PATH BUILD_DIR INIT_SUBMODULE_IF_NEEDED)
	# wzmaplib lives inside the warzone2100 repo
	# Add 3rdparty dependencies of the parent warzone2100 project (optionally with submodule init)
	get_filename_component(_absolute_path "${PATH}" ABSOLUTE)
	if(EXISTS "${_absolute_path}/CMakeLists.txt")
		# Dir contents already initialized - just use add_subdirectory
		message(STATUS "wzmaplib: Using ${PATH}")
		add_subdirectory("${PATH}" "${BUILD_DIR}" EXCLUDE_FROM_ALL)
		return()
	endif()
	if(NOT INIT_SUBMODULE_IF_NEEDED)
		message(FATAL_ERROR "\"${PATH}/CMakeLists.txt\" does not exist, and is not expected to be a submodule that might need init.")
		return()
	endif()
	# If the whole repo was checked out with Git, we might have to initialize the submodule at the top-level/3rdparty
	find_package(Git QUIET)
	if(NOT Git_FOUND)
		message(FATAL_ERROR "Git not found - unable check out 3rdparty submodule.")
		return()
	endif()
	function(CheckFolderIsGitRepo PATH OUTPUT_VAR)
		execute_process(
			COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
			WORKING_DIRECTORY "${PATH}"
			OUTPUT_VARIABLE _test_isDirectoryInGitRepo_Output
			ERROR_VARIABLE _test_isDirectoryInGitRepo_Error
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE
		)
		string(LENGTH "${_test_isDirectoryInGitRepo_Output}" _test_isDirectoryInGitRepo_Output_Len)
		if(_test_isDirectoryInGitRepo_Output_Len GREATER 0)
			set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
		else()
			set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
		endif()
	endfunction()
	# Check if target folder is a git repo (or is in one)
	CheckFolderIsGitRepo("${_absolute_path}" _test_isDirectoryInGitRepo_Error)
	if(NOT _test_isDirectoryInGitRepo_Error)
		message(FATAL_ERROR "\"${PATH}\" is not a git repo (or in a git repo) - unable initialize submodule dir.")
		return()
	endif()
	# Attempt to check out submodule at ${PATH}
	message(STATUS "wzmaplib: Fetching submodule: ${PATH}")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- .
		WORKING_DIRECTORY "${_absolute_path}"
		RESULT_VARIABLE exstatus
	)
	if(NOT exstatus EQUAL 0)
		message(FATAL_ERROR "\"Unable to initialize submodule dir: ${PATH}\"")
		return()
	endif()
	# Re-check for presence of CMakeLists.txt
	if(NOT EXISTS "${_absolute_path}/CMakeLists.txt")
		message(FATAL_ERROR "\"Unable to initialize submodule dir: ${PATH}\" - expected CMakeLists.txt is missing")
		return()
	endif()
	# Submodule dir initialized - use add_subdirectory
	add_subdirectory("${PATH}" "${BUILD_DIR}" EXCLUDE_FROM_ALL)
endfunction()

if(NOT TARGET nlohmann_json)
	message(STATUS "wzmaplib: Fetching nlohmann/json")
	set(JSON_ImplicitConversions OFF CACHE BOOL "Enable implicit conversions." FORCE)
	set(JSON_SystemInclude ON CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)
	WZMAPLIB_ADD_3RDPARTY_DIR("../../3rdparty/json" "3rdparty/json" FALSE)
	# Temporary workaround until we use the new NLOHMANN_JSON_NAMESPACE (etc) macros
	target_compile_definitions(nlohmann_json INTERFACE "NLOHMANN_JSON_NAMESPACE=nlohmann;NLOHMANN_JSON_NAMESPACE_BEGIN=namespace nlohmann {;NLOHMANN_JSON_NAMESPACE_END=}")
endif()
target_link_libraries(wzmaplib PRIVATE nlohmann_json)
if(NOT TARGET optional-lite)
	message(STATUS "wzmaplib: Fetching martinmoene/optional-lite")
	WZMAPLIB_ADD_3RDPARTY_DIR("../../3rdparty/optional-lite" "3rdparty/optional-lite" FALSE)
endif()
target_link_libraries(wzmaplib PUBLIC optional-lite)
if(NOT TARGET quickjs)
	message(STATUS "wzmaplib: Fetching Warzone2100/quickjs-wz")
	WZMAPLIB_ADD_3RDPARTY_DIR("../../3rdparty/quickjs-wz" "3rdparty/quickjs-wz" TRUE)
endif()
target_link_libraries(wzmaplib PRIVATE quickjs)

############
# [Plugins]

##################################
# ZipIOProvider (requires libzip)

set(WZ_LIBZIP_FIND_BASE_PARAMS)
if(VCPKG_TOOLCHAIN)
    set(WZ_LIBZIP_FIND_BASE_PARAMS CONFIG)
endif()
if (WZMAPLIB_ENABLE_ZIPIOPROVIDER)
    list(APPEND WZ_LIBZIP_FIND_BASE_PARAMS REQUIRED)
else()
    list(APPEND WZ_LIBZIP_FIND_BASE_PARAMS QUIET)
endif()
set(WZ_LIBZIP_FIND_ADDITIONAL_PARAMS)
if (APPLE OR WIN32)
    list(APPEND WZ_LIBZIP_FIND_ADDITIONAL_PARAMS NO_SYSTEM_ENVIRONMENT_PATH)
endif()
find_package(libzip ${WZ_LIBZIP_FIND_BASE_PARAMS} ${WZ_LIBZIP_FIND_ADDITIONAL_PARAMS})
if (libzip_FOUND)
	message(STATUS "Found libzip version: ${libzip_VERSION}")
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
	if (libzip_VERSION VERSION_LESS 1.10)
		# libzip < 1.10 exported CMake config does not currently include all required support libraries - see: https://github.com/nih-at/libzip/issues/205
		# So manually find and link them
		find_package (ZLIB REQUIRED)
		list(APPEND CMAKE_REQUIRED_LIBRARIES "ZLIB::ZLIB")
		find_package (BZip2)
		if (BZip2_FOUND)
			list(APPEND CMAKE_REQUIRED_LIBRARIES "BZip2::BZip2")
		endif()
	endif()
	check_cxx_source_compiles("${_test_libzip_source}" libzip_LINK_TEST)
	cmake_pop_check_state()
	if(libzip_LINK_TEST)
		message( STATUS "Found libzip... (link test successful)" )
	else()
		message( WARNING "Found libzip, but link test NOT successful? (Continuing anyway)" )
	endif()
	get_target_property(_libzip_INCLUDE_DIR libzip::zip INTERFACE_INCLUDE_DIRECTORIES)
	message( STATUS "- libzip include dir: ${_libzip_INCLUDE_DIR}" )
endif()
if (libzip_FOUND)
	file(GLOB ZIPIOPROVIDER_HEADERS "plugins/ZipIOProvider/include/ZipIOProvider.h")
	file(GLOB ZIPIOPROVIDER_SRC "plugins/ZipIOProvider/src/ZipIOProvider.cpp")

	add_library(ZipIOProvider STATIC ${ZIPIOPROVIDER_HEADERS} ${ZIPIOPROVIDER_SRC})
	set_property(TARGET ZipIOProvider PROPERTY FOLDER "lib")
	set_target_properties(ZipIOProvider
		PROPERTIES
			CXX_STANDARD 17
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
	if (libzip_VERSION VERSION_LESS 1.10)
		# libzip exported CMake config does not currently include all required support libraries - see: https://github.com/nih-at/libzip/issues/205
		# So manually find and link them
		find_package (ZLIB REQUIRED)
		target_link_libraries(ZipIOProvider PRIVATE ZLIB::ZLIB)
		find_package (BZip2)
		if (BZip2_FOUND)
			target_link_libraries(ZipIOProvider PRIVATE BZip2::BZip2)
		endif()
	endif()
else()
	message(WARNING "Could NOT find libzip - ZipIOProvider target will not be available")
endif()
