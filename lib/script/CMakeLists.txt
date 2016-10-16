cmake_minimum_required (VERSION 3.5)
project (script CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(script ${HEADERS} ${SRC})
target_link_libraries(script Qt5::Core)
