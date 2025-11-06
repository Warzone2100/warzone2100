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

# Find SDL3
find_package(SDL3 3.2.12 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

add_library(sdl-backend STATIC ${HEADERS} ${SRC})
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sdl-backend)

target_link_libraries(sdl-backend PRIVATE framework ivis-opengl optional-lite)
set_property(TARGET sdl-backend PROPERTY FOLDER "lib")

message( STATUS "Linking to SDL3 library" )
target_link_libraries(sdl-backend PRIVATE SDL3::SDL3)
target_compile_definitions(sdl-backend PRIVATE "-DHAVE_SDL_VULKAN_H")
