cmake_minimum_required(VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")
qt5_wrap_cpp(MOCFILES multiint.h qtscriptdebug.h)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    COMMAND "powershell.exe"
    ARGS "-File" "${CMAKE_SOURCE_DIR}/build_tools/autorevision.ps1" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    COMMAND "${CMAKE_SOURCE_DIR}/build_tools/autorevision"
    ARGS "-t" "h" "-o" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" ">" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h")
endif()

add_executable(warzone2100 ${HEADERS} ${SRC} ${MOCFILES} "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h" "${CMAKE_SOURCE_DIR}/win32/warzone2100.rc")
target_compile_definitions(warzone2100 PRIVATE "YY_NO_UNISTD_H")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(warzone2100 exception-handler gamelib ivis-opengl netplay script sdl-backend framework sequence sound widget)
target_link_libraries(warzone2100
	Qt5::Script
	Qt5::Gui
	Qt5::Widgets
	)

if(WIN32)
    target_link_libraries(warzone2100 ws2_32 iphlpapi ${SDL2MAIN_LIBRARY})
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_libraries(warzone2100 ogg vorbis vorbisfile theora)
  if (${CMAKE_CROSSCOMPILING})
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group,-subsystem,windows")
    target_link_libraries(warzone2100 ${QT5ALL_LIBRARIES} ${QT5ALL_LDFLAGS_OTHER})
    target_link_libraries(warzone2100 version Qt5::QWindowsIntegrationPlugin Qt5ThemeSupport Qt5FontDatabaseSupport Qt5EventDispatcherSupport)
  else()
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group")
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    target_link_libraries(warzone2100
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/SDL2.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/ogg.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/vorbis.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/vorbisfile.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/theora.lib"
    )
endif()

install(TARGETS warzone2100 DESTINATION ".")
