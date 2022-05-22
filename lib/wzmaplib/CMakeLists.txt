cmake_minimum_required(VERSION 3.5...3.14)
project (wzmaplib CXX)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

list(APPEND CMAKE_MODULE_PATH "../../cmake")

file(GLOB PUBLIC_HEADERS "include/wzmaplib/*.h")
file(GLOB PRIVATE_HEADERS "src/*.h")
file(GLOB SRC "src/*.cpp")

add_library(wzmaplib STATIC ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SRC})
set_property(TARGET wzmaplib PROPERTY FOLDER "lib")
include(WZTargetConfiguration OPTIONAL RESULT_VARIABLE _wztargetconfiguration_module_path)
if(_wztargetconfiguration_module_path)
	WZ_TARGET_CONFIGURATION(wzmaplib)
else()
	message(STATUS "Unable to find WZTargetConfiguration - continuing without")
endif()
if(MSVC)
	target_compile_definitions(wzmaplib PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
target_include_directories(wzmaplib PUBLIC "include")
# for optional-lite + nlohmann::json header-only libraries
target_include_directories(wzmaplib PUBLIC "../../3rdparty/")
target_compile_definitions(wzmaplib PRIVATE "JSON_USE_IMPLICIT_CONVERSIONS=0")

if(NOT TARGET quickjs)
	add_subdirectory("../../3rdparty/quickjs" "3rdparty/quickjs" EXCLUDE_FROM_ALL)
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
