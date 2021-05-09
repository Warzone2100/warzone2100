cmake_minimum_required(VERSION 3.5...3.14)
project (maptools CXX)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

if(NOT TARGET maplib)
	# Not embedded in parent main WZ build system - must manually include maplib
	add_subdirectory("../../lib/maplib" "lib/maplib" EXCLUDE_FROM_ALL)
endif()

find_package(PNG 1.2 REQUIRED)

set (CMAKE_USE_PTHREADS_INIT TRUE)
find_package(Threads)

# set CMAKE_BUILD_TYPE to default
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif ()

# maptools
add_executable(maptools
				maptools.cpp pngsave.cpp pngsave.h)
target_link_libraries(maptools PRIVATE maplib PNG::PNG)

if(MSVC)
	target_compile_definitions(maptools PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
