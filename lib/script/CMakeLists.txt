cmake_minimum_required (VERSION 3.5)
project (script CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(script STATIC ${HEADERS} ${SRC})
target_link_libraries(script PRIVATE framework)
target_compile_definitions(script PRIVATE "YY_NO_UNISTD_H")
