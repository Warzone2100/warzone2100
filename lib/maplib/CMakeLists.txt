cmake_minimum_required(VERSION 3.5...3.14)
project (maplib CXX)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(maplib STATIC ${HEADERS} ${SRC})
set_property(TARGET maplib PROPERTY FOLDER "lib")
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(maplib PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()
if(MSVC)
	target_compile_definitions(maplib PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
# for optional-lite + nlohmann::json header-only libraries
target_include_directories(maplib PRIVATE "../../3rdparty/")

if(NOT TARGET quickjs)
	add_subdirectory("../../3rdparty/quickjs" "3rdparty/quickjs" EXCLUDE_FROM_ALL)
endif()
target_link_libraries(maplib PRIVATE quickjs)
