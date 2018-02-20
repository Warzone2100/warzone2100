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
    ARGS "-t" "h" "-o" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" ">" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

add_executable(warzone2100 ${HEADERS} ${SRC} ${MOCFILES} "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h" "${CMAKE_SOURCE_DIR}/win32/warzone2100.rc")
target_compile_definitions(warzone2100 PRIVATE "YY_NO_UNISTD_H")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
if(WZ_PORTABLE)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES OUTPUT_NAME "warzone2100_portable")
endif()

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
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
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

#######################
# Hardening / Security

INCLUDE(AddTargetLinkFlagsIfSupported)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Enable Data Execution Prevention and Address Space Layout Randomization

	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/NXCOMPAT" CACHED_RESULT_NAME LINK_FLAG_SLASH_NXCOMPAT_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--nxcompat" CACHED_RESULT_NAME LINK_FLAG_WL_NXCOMPAT_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/DYNAMICBASE" CACHED_RESULT_NAME LINK_FLAG_SLASH_DYNAMICBASE_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--dynamicbase" CACHED_RESULT_NAME LINK_FLAG_WL_DYNAMICBASE_SUPPORTED)
endif()

# Enable RELRO (if supported)
ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,relro" CACHED_RESULT_NAME LINK_FLAG_WL_Z_RELRO_SUPPORTED)
ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,now" CACHED_RESULT_NAME LINK_FLAG_WL_Z_NOW_SUPPORTED)

#######################
# Install
install(TARGETS warzone2100 COMPONENT Core DESTINATION ".")
