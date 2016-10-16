cmake_minimum_required (VERSION 3.5)
project (gamelib CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(gamelib ${HEADERS} ${SRC})
target_link_libraries(gamelib Qt5::Core)
