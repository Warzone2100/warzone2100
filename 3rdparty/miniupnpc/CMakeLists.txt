cmake_minimum_required (VERSION 3.5)
project (miniupnpc C)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.c")

add_library(miniupnpc ${HEADERS} ${SRC})

#install(TARGETS hello-world DESTINATION bin)