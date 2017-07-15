cmake_minimum_required (VERSION 3.5)
project (sdl-backend CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

find_package(SDL2 REQUIRED)

add_library(sdl-backend ${HEADERS} ${SRC})
target_include_directories(sdl-backend PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(sdl-backend PRIVATE framework ${SDL2_LIBRARY})
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  if(${CMAKE_CROSSCOMPILING})
    target_compile_definitions(sdl-backend PRIVATE "QT_STATICPLUGIN")
  endif()
endif()
