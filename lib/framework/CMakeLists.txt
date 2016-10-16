cmake_minimum_required (VERSION 3.5)
project (framework CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(framework ${HEADERS} ${SRC})
target_link_libraries(framework Qt5::Core)
