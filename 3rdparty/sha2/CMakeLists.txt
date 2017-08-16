cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.c")

add_library(sha2 STATIC ${HEADERS} ${SRC})
target_include_directories(sha2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
