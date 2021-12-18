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

if(NOT TARGET quickjs)
	add_subdirectory("../../3rdparty/quickjs" "3rdparty/quickjs" EXCLUDE_FROM_ALL)
endif()
target_link_libraries(wzmaplib PRIVATE quickjs)
