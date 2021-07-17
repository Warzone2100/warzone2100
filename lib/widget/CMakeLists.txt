file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(widget STATIC ${HEADERS} ${SRC})
set_property(TARGET widget PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(widget)
target_link_libraries(widget PRIVATE framework ivis-opengl)
