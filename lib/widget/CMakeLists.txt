file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(widget STATIC ${HEADERS} ${SRC})
set_property(TARGET widget PROPERTY FOLDER "lib")
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(widget PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()
target_link_libraries(widget PRIVATE framework ivis-opengl)
