cmake_minimum_required (VERSION 3.5)
project (gamelib CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(gamelib STATIC ${HEADERS} ${SRC})
target_link_libraries(gamelib PRIVATE framework)
target_compile_definitions(gamelib PRIVATE "YY_NO_UNISTD_H")
