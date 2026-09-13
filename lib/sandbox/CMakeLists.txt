###############################################
# WZ process hardening & filesystem confinement

file(GLOB HEADERS "*.h")
set(SRC "sandbox_common.cpp")

add_library(sandbox STATIC ${HEADERS} ${SRC})
set_property(TARGET sandbox PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sandbox)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_sources(sandbox PRIVATE "sandbox_win.cpp")
elseif(APPLE)
	target_sources(sandbox PRIVATE "sandbox_mac.cpp")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_sources(sandbox PRIVATE "sandbox_linux.cpp")
else()
	target_sources(sandbox PRIVATE "sandbox_null.cpp")
endif()
