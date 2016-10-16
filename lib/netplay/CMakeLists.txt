cmake_minimum_required (VERSION 3.5)
project (netplay CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")
file(GLOB MINIUPNPC "../../3rdparty/miniupnpc/*.c")

add_library(netplay ${HEADERS} ${SRC} ${MINIUPNPC})
target_link_libraries(netplay Qt5::Core)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	target_link_libraries(netplay pthread)
endif()
