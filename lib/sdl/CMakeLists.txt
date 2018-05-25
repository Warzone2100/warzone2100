cmake_minimum_required (VERSION 3.5)
project (sdl-backend CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	list(APPEND SRC "cocoa_sdl_helpers.mm" "swapinterval.mm" "cocoa_wz_menus.mm")
endif()

set(SDL2_MIN_VERSION "2.0.5")

set(_sdl2_library)
set(_sdl2_include_dir)
set(_sdl2_main_library)

# Prefer finding SDL2 using CMake Config, to properly include any dependencies / linked libraries (with complete, imported targets)
# - This should work for vcpkg-installed SDL2, as well as other installs that generate a full (proper) CMake Config,
# - and is required to properly link with a static SDL2 library (at least on Windows and macOS)
find_package(SDL2 ${SDL2_MIN_VERSION} CONFIG QUIET)
if(SDL2_FOUND)
	if (TARGET SDL2::SDL2-static)
		set(_sdl2_library SDL2::SDL2-static)
	elseif(TARGET SDL2::SDL2)
		set(_sdl2_library SDL2::SDL2)
	else()
		# Fall-back to FindSDL2 module (below)
		set(_sdl2_main_library)
	endif()
	if (TARGET SDL2::SDL2main)
		set(_sdl2_main_library SDL2::SDL2main)
	endif()

	if(_sdl2_library)
		get_target_property(_sdl2_target_include_dir ${_sdl2_library} INTERFACE_INCLUDE_DIRECTORIES)
		set(_sdl2_include_dir "${_sdl2_target_include_dir}/SDL2") # WORKAROUND
	endif()
endif()

if(NOT _sdl2_library OR NOT SDL2_FOUND)
	# Fall-back to using the FindSDL2 module
	message( STATUS "Using FindSDL2 module" )
	find_package(SDL2 ${SDL2_MIN_VERSION} MODULE REQUIRED)

	set(_sdl2_library "${SDL2_LIBRARY}")
	set(_sdl2_include_dir "${SDL2_INCLUDE_DIR}")
	set(_sdl2_main_library "${SDL2MAIN_LIBRARY}")
endif()

add_library(sdl-backend ${HEADERS} ${SRC})
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(sdl-backend PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()

target_link_libraries(sdl-backend PRIVATE framework)
message( STATUS "Linking to SDL2 library: ${_sdl2_library}" )
target_link_libraries(sdl-backend PRIVATE ${_sdl2_library})
target_include_directories(sdl-backend PRIVATE "${_sdl2_include_dir}")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${_sdl2_main_library}" STREQUAL "")
		message( WARNING "SDL2Main library not found. Linking will not succeed." )
	endif()
endif()
message( STATUS "Main target should link to SDL2Main: ${_sdl2_main_library}" )
target_link_libraries(sdl-backend INTERFACE ${_sdl2_main_library})

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  if(${CMAKE_CROSSCOMPILING})
    target_compile_definitions(sdl-backend PRIVATE "QT_STATICPLUGIN")
  endif()
endif()
