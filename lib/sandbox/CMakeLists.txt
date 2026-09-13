###############################################
# WZ process hardening & filesystem confinement

file(GLOB HEADERS "*.h")
set(SRC "sandbox_common.cpp")

add_library(sandbox STATIC ${HEADERS} ${SRC})
set_property(TARGET sandbox PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sandbox)

if(APPLE)
	target_sources(sandbox PRIVATE "sandbox_mac.cpp")
else()
	target_sources(sandbox PRIVATE "sandbox_null.cpp")
endif()

target_link_libraries(sandbox PRIVATE framework)
