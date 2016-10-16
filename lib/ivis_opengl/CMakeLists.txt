cmake_minimum_required (VERSION 3.5)
project (ivis-opengl CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  pkg_check_modules(HARFBUZZ harfbuzz)
else()
  set(HARFBUZZ_INCLUDE_DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/harfbuzz)
  set(HARFBUZZ_LIBRARIES ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/harfbuzz.lib)
endif()

add_library(ivis-opengl ${HEADERS} ${SRC})
target_include_directories(ivis-opengl PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_link_libraries(ivis-opengl Qt5::Core ${HARFBUZZ_LIBRARIES})
