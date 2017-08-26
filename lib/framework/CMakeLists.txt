cmake_minimum_required (VERSION 3.5)

find_package(PhysFS REQUIRED)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(framework STATIC ${HEADERS} ${SRC})
target_link_libraries(framework PUBLIC Qt5::Core ${PHYSFS_LIBRARY})
target_link_libraries(framework PRIVATE microecc sha2)
target_include_directories(framework PUBLIC "${PHYSFS_INCLUDE_DIR}" "${CMAKE_SOURCE_DIR}/3rdparty")
target_compile_definitions(framework PRIVATE "YY_NO_UNISTD_H")
target_compile_definitions(framework PUBLIC "_CRT_SECURE_NO_WARNINGS")
