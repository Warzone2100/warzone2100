if(NOT (CMAKE_SYSTEM_NAME MATCHES "Windows" OR CMAKE_SYSTEM_NAME MATCHES "Darwin"))
	# Temporary workaround for systems with broken SDL2 CMake config (ex. imported target does not exist in "SDL2Targets.cmake")
	# Can be manually set to "ON" to avoid using the CONFIG mode
	OPTION(WZ_FINDSDL2_NOCONFIG "Disable trying to find SDL2 using CONFIG mode" OFF)
endif()

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	list(APPEND SRC "cocoa_sdl_helpers.mm" "cocoa_wz_menus.mm")
endif()

set(SDL2_MIN_VERSION "2.0.5")

set(_sdl2_library)
set(_sdl2_include_dir)
set(_sdl2_main_library)

function(test_link_to_sdl_target sdl_target sdl_main output_var)

	set(CMAKE_REQUIRED_LIBRARIES "${sdl_target}")
	set(_additional_includes "")
	if (DEFINED sdl_main AND sdl_main AND NOT "${sdl_main}" STREQUAL "")
		list(APPEND CMAKE_REQUIRED_LIBRARIES ${sdl_main})
		set(_additional_includes "#include <SDL_main.h>")
	endif()
	check_cxx_source_compiles(
	   "#include <SDL_version.h>
		${_additional_includes}
		#include <cstdio>

		void print_sdl_version() {
		  SDL_version linked_sdl_version;
		  SDL_GetVersion(&linked_sdl_version);
		  printf(\"Linked SDL version: %u.%u.%u\", (unsigned int)linked_sdl_version.major, (unsigned int)linked_sdl_version.minor, (unsigned int)linked_sdl_version.patch);
		}

		int main(int argc, char *argv[])
		{
		  print_sdl_version();
		  return 0;
		}"
	  ${output_var}
	)
	set(CMAKE_REQUIRED_LIBRARIES "")

    set(${output_var} "${${output_var}}" PARENT_SCOPE)
endfunction()

# Prefer finding SDL2 using CMake Config, to properly include any dependencies / linked libraries (with complete, imported targets)
# - This should work for vcpkg-installed SDL2, as well as other installs that generate a full (proper) CMake Config,
# - and is required to properly link with a static SDL2 library (at least on Windows and macOS)
if(NOT DEFINED WZ_FINDSDL2_NOCONFIG OR NOT WZ_FINDSDL2_NOCONFIG)
	find_package(SDL2 ${SDL2_MIN_VERSION} CONFIG QUIET)
endif()
if(SDL2_FOUND)
	if (TARGET SDL2::SDL2-static)
		# Check for INTERFACE_INCLUDE_DIRECTORIES to ignore the "aliased" target that vcpkg creates
		get_target_property(_sdl2_include_dir SDL2::SDL2-static INTERFACE_INCLUDE_DIRECTORIES)
		test_link_to_sdl_target(SDL2::SDL2-static FALSE LINK_SUCCESS_SDL2_SDL2STATIC_TARGET)
		if (_sdl2_include_dir AND LINK_SUCCESS_SDL2_SDL2STATIC_TARGET)
			set(_sdl2_library SDL2::SDL2-static)
		endif()
	endif()
	if (NOT _sdl2_library AND TARGET SDL2::SDL2)
		# Check for INTERFACE_INCLUDE_DIRECTORIES to ignore the "aliased" target that vcpkg creates
		get_target_property(_sdl2_include_dir SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
		test_link_to_sdl_target(SDL2::SDL2 FALSE LINK_SUCCESS_SDL2_SDL2_TARGET)
		if (_sdl2_include_dir AND LINK_SUCCESS_SDL2_SDL2_TARGET)
			set(_sdl2_library SDL2::SDL2)
		endif()
	endif()
	if (NOT _sdl2_library)
		# Fall-back to FindSDL2 module (below)
	elseif (TARGET SDL2::SDL2main)
		test_link_to_sdl_target("${_sdl2_library}" SDL2::SDL2main LINK_SUCCESS_SDL2_SDL2MAIN_TARGET)
		if (LINK_SUCCESS_SDL2_SDL2MAIN_TARGET)
			set(_sdl2_main_library SDL2::SDL2main)
		endif()
	endif()

	if(_sdl2_library)
		if(VCPKG_TOOLCHAIN)
			# vcpkg workaround to link the Release version of SDL2 on Windows
			# for MinSizeRel and RelWithDebInfo configs
			set_target_properties(${_sdl2_library} PROPERTIES
				MAP_IMPORTED_CONFIG_MINSIZEREL Release
				MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
			)
			message(STATUS "Mapped imported configs for target: ${_sdl2_library}")
			if (TARGET SDL2::SDL2main)
				set_target_properties(SDL2::SDL2main PROPERTIES
					MAP_IMPORTED_CONFIG_MINSIZEREL Release
					MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
				)
			endif()
		endif()

		get_target_property(_sdl2_include_dir ${_sdl2_library} INTERFACE_INCLUDE_DIRECTORIES)
		if (NOT _sdl2_include_dir)
			message(WARNING "Failed to find INTERFACE_INCLUDE_DIRECTORIES in SDL2 target: ${_sdl2_library}")
		endif()
		get_filename_component(_last_directory "${_sdl2_include_dir}" NAME)
		if (NOT _last_directory STREQUAL "SDL2")
			set(_sdl2_include_dir "${_sdl2_include_dir}/SDL2") # WORKAROUND
		endif()
	endif()
endif()

if(NOT _sdl2_library OR NOT SDL2_FOUND)
	# Fall-back to using the FindSDL2 module
	message( STATUS "Using FindSDL2 module" )
	find_package(SDL2 ${SDL2_MIN_VERSION} MODULE REQUIRED)

	set(_sdl2_library "${SDL2_LIBRARY}")
	set(_sdl2_include_dir "${SDL2_INCLUDE_DIR}")
	if(SDL2MAIN_FOUND)
		set(_sdl2_main_library "${SDL2MAIN_LIBRARY}")
	endif()
endif()

# Check if SDL2's "SDL_vulkan.h" is available (SDL 2.0.6+)
INCLUDE (CheckIncludeFileCXX)
set(_prior_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${_sdl2_include_dir}")
CHECK_INCLUDE_FILE_CXX("SDL_vulkan.h" HAVE_SDL_VULKAN_H)
set(CMAKE_REQUIRED_INCLUDES "${_prior_CMAKE_REQUIRED_INCLUDES}")

add_library(sdl-backend STATIC ${HEADERS} ${SRC})
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sdl-backend)

target_link_libraries(sdl-backend PRIVATE framework ivis-opengl optional-lite)
set_property(TARGET sdl-backend PROPERTY FOLDER "lib")
message( STATUS "Linking to SDL2 library: ${_sdl2_library} (version ${SDL2_VERSION})" )
target_link_libraries(sdl-backend PRIVATE ${_sdl2_library})
target_include_directories(sdl-backend PRIVATE "${_sdl2_include_dir}")
if(HAVE_SDL_VULKAN_H)
	target_compile_definitions(sdl-backend PRIVATE "-DHAVE_SDL_VULKAN_H")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${_sdl2_main_library}" STREQUAL "")
		message( WARNING "SDL2Main library not found. Linking will not succeed." )
	endif()
endif()
if(_sdl2_main_library)
	message( STATUS "Main target should link to SDL2Main: ${_sdl2_main_library}" )
	target_link_libraries(sdl-backend INTERFACE ${_sdl2_main_library})
else()
	message( STATUS "Not linking to SDL2Main" )
endif()
