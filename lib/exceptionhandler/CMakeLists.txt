###############################################
# WZ core / default exception handler (writes local logs & dumps)
file(GLOB HEADERS "*.h")
set(SRC "dumpinfo.cpp" "exceptionhandler.cpp")

add_library(exception-handler STATIC ${HEADERS} ${SRC})
set_property(TARGET exception-handler PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(exception-handler)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
		# Cross-compiling for Windows with MINGW
		target_sources(exception-handler PRIVATE "exchndl_mingw.cpp")
		target_link_libraries(exception-handler PRIVATE bfd iberty)
	else()
		# Compiling with MSVC (etc) on Windows
		target_sources(exception-handler PRIVATE "3rdparty/StackWalker.cpp" "exchndl_win.cpp")
		target_link_libraries(exception-handler PRIVATE dbghelp version shlwapi)
		if (MINGW AND NOT MSVC)
			# Disable some warnings
			set_source_files_properties("3rdparty/StackWalker.cpp" PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas -Wno-switch")
			# Define _MSC_VER
			set_source_files_properties("3rdparty/StackWalker.cpp" PROPERTIES COMPILE_DEFINITIONS "_MSC_VER=1900")
		endif()
		set_property(SOURCE "3rdparty/StackWalker.h" "3rdparty/StackWalker.cpp" APPEND PROPERTY COMPILE_DEFINITIONS "WIN32_LEAN_AND_MEAN" "NOCRYPT" "NOMINMAX" "WIN32_EXTRA_LEAN")
	endif()
endif()

target_link_libraries(exception-handler PRIVATE framework launchinfo)

###############################################
# WZ Sentry.io crash handling provider support
# To build, define CMAKE variables:
#  - WZ_BUILD_SENTRY (to build with sentry support)
#  - SENTRY_IO_DSN (to supply the DSN configuration URL)

if(DEFINED WZ_BUILD_SENTRY AND WZ_BUILD_SENTRY)
	include(FetchSentryNative) # uses FetchContent, so requires CMake 3.11+

	# Piggy-back on the exception-handler target to get these libraries / defines over to the main target # TODO: Refactor later?
	target_link_libraries(exception-handler INTERFACE sentry)
	target_compile_definitions(exception-handler INTERFACE WZ_CRASHHANDLING_PROVIDER_SENTRY)
	target_compile_definitions(exception-handler INTERFACE SENTRY_BUILD_STATIC=1)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config")
	if(DEFINED SENTRY_IO_DSN)
		set(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN "${SENTRY_IO_DSN}")
	else()
		unset(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN)
	endif()
	set(WZ_CRASHPAD_HANDLER_OUTPUT_NAME "crashpad_handler")
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		# On Linux, give the crashpad_handler a unique, WZ-specific name
		set(WZ_CRASHPAD_HANDLER_OUTPUT_NAME "crashpad_handler_warzone2100${WZ_OUTPUT_NAME_SUFFIX}")
	endif()
	set(WZ_CRASHPAD_EXECUTABLE_NAME "${WZ_CRASHPAD_HANDLER_OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
	if(TARGET crashpad_handler)
		set_target_properties(crashpad_handler PROPERTIES SUFFIX "${CMAKE_EXECUTABLE_SUFFIX}")
		set_target_properties(crashpad_handler PROPERTIES OUTPUT_NAME "${WZ_CRASHPAD_HANDLER_OUTPUT_NAME}")
		message(STATUS "WZ_CRASHPAD_EXECUTABLE_NAME=${WZ_CRASHPAD_EXECUTABLE_NAME}")
	endif()
	configure_file("3rdparty/sentry/wz-sentry-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config/wz-sentry-config.h" @ONLY)
	target_include_directories(exception-handler INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config")
endif()
