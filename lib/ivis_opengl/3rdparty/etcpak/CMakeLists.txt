file(GLOB HEADERS "*.hpp")
file(GLOB SRC "*.cpp")

add_library(etcpak STATIC ${HEADERS} ${SRC})
set_property(TARGET etcpak PROPERTY FOLDER "3rdparty")
target_include_directories(etcpak INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_compile_definitions(etcpak PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
