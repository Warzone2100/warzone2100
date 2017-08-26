cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.c")

add_library(microecc STATIC ${HEADERS} ${SRC})
target_include_directories(microecc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
