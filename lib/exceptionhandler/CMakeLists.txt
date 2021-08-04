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
	if(CMAKE_SYSTEM_NAME MATCHES "Windows")
		# Sentry-native requires CMAKE_SYSTEM_VERSION to be properly set
		if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
			set(CMAKE_SYSTEM_VERSION "10")
		else()
			set(CMAKE_SYSTEM_VERSION "6.0") # Windows Vista+
		endif()
	endif()
	if(MINGW AND ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
		# Sentry-native currently requires uasm when compiling via mingw-clang
		if(NOT DEFINED CMAKE_ASM_MASM_COMPILER)
			set(CMAKE_ASM_MASM_COMPILER "uasm")
		endif()
	endif()
	include(FetchContent)
	FetchContent_Declare(
		sentrynative
		URL "https://github.com/getsentry/sentry-native/releases/download/0.4.12/sentry-native.zip"
		URL_HASH SHA512=15da4407ed5e2c8d5e56e497ccc6006b29235aef6b3a81e034c93443e20a7cfdf95d55e31b88e552c55e824eb15d6f7fafe988c453a5a6f36fe45136d7268b19
	)
	FetchContent_GetProperties(sentrynative)
	set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "Sentry build shared libs" FORCE)
	set(SENTRY_EXPORT_SYMBOLS OFF CACHE BOOL "Export symbols" FORCE)
	set(SENTRY_BUILD_RUNTIMESTATIC OFF CACHE BOOL "Build sentry-native with static runtime" FORCE)
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
		set(SENTRY_BACKEND "breakpad" CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'." FORCE)
	endif()
	if(NOT sentrynative_POPULATED)
		FetchContent_Populate(sentrynative)
		add_subdirectory("${sentrynative_SOURCE_DIR}" "${sentrynative_BINARY_DIR}")
	endif()
	message(STATUS "Enabling crash-handling backend: sentry-native ($CACHE{SENTRY_BACKEND})")

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
	configure_file("3rdparty/sentry/wz-sentry-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config/wz-sentry-config.h" @ONLY)
	target_include_directories(exception-handler INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config")
endif()
