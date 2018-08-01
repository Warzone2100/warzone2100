cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

find_package (Threads REQUIRED)

add_library(netplay STATIC ${HEADERS} ${SRC})
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(netplay PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()
target_link_libraries(netplay PRIVATE framework)
if(WZ_USE_IMPORTED_MINIUPNPC)
	target_link_libraries(netplay PRIVATE imported-miniupnpc)
else()
	# Link with in-tree miniupnpc
	target_link_libraries(netplay PRIVATE libminiupnpc-static)
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	target_link_libraries(netplay PRIVATE pthread)
endif()
target_link_libraries(netplay PRIVATE Threads::Threads)
