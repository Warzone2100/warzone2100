if(WIN32)
	cmake_minimum_required (VERSION 3.16.4)

	# enables support for CMAKE_MSVC_RUNTIME_LIBRARY
	cmake_policy(SET CMP0091 NEW)
else()
	# The Android tools ship with this ancient version, which we need to support.
	cmake_minimum_required (VERSION 3.10)
endif()

#read sentry-native version
file(READ "include/sentry.h" _SENTRY_HEADER_CONTENT)
string(REGEX MATCH "#define SENTRY_SDK_VERSION \"([0-9\.]+)\"" _SENTRY_VERSION_MATCH "${_SENTRY_HEADER_CONTENT}")
set(SENTRY_VERSION "${CMAKE_MATCH_1}")
unset(_SENTRY_HEADER_CONTENT)
unset(_SENTRY_VERSION_MATCH)

project(Sentry-Native
	LANGUAGES C CXX ASM
	VERSION ${SENTRY_VERSION}
)

set(SENTRY_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(SENTRY_MAIN_PROJECT ON)
endif()

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/sentry")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LINUX TRUE)
endif()

#setup sentry library type
if(SENTRY_MAIN_PROJECT AND NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()
option(SENTRY_BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ${BUILD_SHARED_LIBS})
if(SENTRY_BUILD_SHARED_LIBS)
	set(SENTRY_LIBRARY_TYPE SHARED)
else()
	set(SENTRY_LIBRARY_TYPE STATIC)
endif()

option(SENTRY_PIC "Build sentry (and dependent) libraries as position independent libraries" ON)

option(SENTRY_BUILD_TESTS "Build sentry-native tests" "${SENTRY_MAIN_PROJECT}")
option(SENTRY_BUILD_EXAMPLES "Build sentry-native example(s)" "${SENTRY_MAIN_PROJECT}")

option(SENTRY_LINK_PTHREAD "Link platform threads library" ON)
if(SENTRY_LINK_PTHREAD)
	find_package(Threads REQUIRED)
endif()

if(MSVC)
	option(SENTRY_BUILD_RUNTIMESTATIC "Build sentry-native with static runtime" OFF)
endif()

if(LINUX)
	option(SENTRY_BUILD_FORCE32 "Force a 32bit compile on a 64bit host" OFF)
	if(SENTRY_BUILD_FORCE32)
		set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
		set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
		set(CMAKE_ASM_FLAGS  "${CMAKE_ASM_FLAGS} -m32 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
		set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
	endif()
endif()

# CMAKE_POSITION_INDEPENDENT_CODE must be set BEFORE adding any libraries (including subprojects)
if(SENTRY_PIC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
	set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
endif()

if(WIN32)
	set(SENTRY_DEFAULT_TRANSPORT "winhttp")
elseif((APPLE AND NOT IOS) OR LINUX)
	set(SENTRY_DEFAULT_TRANSPORT "curl")
else()
	set(SENTRY_DEFAULT_TRANSPORT "none")
endif()

set(SENTRY_TRANSPORT ${SENTRY_DEFAULT_TRANSPORT} CACHE STRING
  "The HTTP transport that sentry uses to submit events to the sentry server, can be either 'none', 'curl' or 'winhttp' on windows.")

if(SENTRY_TRANSPORT STREQUAL "winhttp")
	set(SENTRY_TRANSPORT_WINHTTP TRUE)
elseif(SENTRY_TRANSPORT STREQUAL "curl")
	set(SENTRY_TRANSPORT_CURL TRUE)
elseif(SENTRY_TRANSPORT STREQUAL "none")
	set(SENTRY_TRANSPORT_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_TRANSPORT must be one of 'none', 'curl' or 'winhttp'")
endif()

if(SENTRY_TRANSPORT_WINHTTP AND NOT WIN32)
	message(FATAL_ERROR "The winhttp transport is only supported on Windows.")
endif()

if(SENTRY_BUILD_TESTS OR SENTRY_BUILD_EXAMPLES)
	enable_testing()
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	set(SENTRY_MAIN_PROJECT ON)
endif()

option(SENTRY_ENABLE_INSTALL "Enable sentry installation" "${SENTRY_MAIN_PROJECT}")

if(MSVC AND CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
	message(WARNING "Crashpad is not supported for MSVC with XP toolset. Default backend was switched to 'breakpad'")
	set(SENTRY_DEFAULT_BACKEND "breakpad")
elseif((APPLE AND NOT IOS) OR WIN32)
	set(SENTRY_DEFAULT_BACKEND "crashpad")
elseif(LINUX)
	set(SENTRY_DEFAULT_BACKEND "breakpad")
else()
	set(SENTRY_DEFAULT_BACKEND "inproc")
endif()

set(SENTRY_BACKEND ${SENTRY_DEFAULT_BACKEND} CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'.")

if(SENTRY_BACKEND STREQUAL "crashpad")
	set(SENTRY_BACKEND_CRASHPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "inproc")
	set(SENTRY_BACKEND_INPROC TRUE)
elseif(SENTRY_BACKEND STREQUAL "breakpad")
	set(SENTRY_BACKEND_BREAKPAD TRUE)
elseif(SENTRY_BACKEND STREQUAL "none")
	set(SENTRY_BACKEND_NONE TRUE)
else()
	message(FATAL_ERROR "SENTRY_BACKEND must be one of 'crashpad', 'inproc', 'breakpad' or 'none'")
endif()

if(SENTRY_BACKEND_CRASHPAD AND ANDROID)
	message(FATAL_ERROR "The Crashpad backend is not currently supported on Android")
endif()

message(STATUS "SENTRY_TRANSPORT=${SENTRY_TRANSPORT}")
message(STATUS "SENTRY_BACKEND=${SENTRY_BACKEND}")
message(STATUS "SENTRY_LIBRARY_TYPE=${SENTRY_LIBRARY_TYPE}")

if(ANDROID)
	set(SENTRY_WITH_LIBUNWINDSTACK TRUE)
elseif(NOT WIN32)
	set(SENTRY_WITH_LIBBACKTRACE TRUE)
endif()

option(WITH_ASAN_OPTION "Build sentry-native with address sanitizer" OFF)
if(WITH_ASAN_OPTION)
	add_compile_options(-g -fsanitize=address -fno-omit-frame-pointer)
	link_libraries(-fsanitize=address)
endif()

option(WITH_TSAN_OPTION "Build sentry-native with thread sanitizer" OFF)
if(WITH_TSAN_OPTION)
	add_compile_options(-g -fsanitize=thread -fno-omit-frame-pointer)
	link_libraries(-fsanitize=thread)
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# use -O3 when doing `RelWithDebInfo` builds
if(NOT MSVC)
	foreach(lang ASM C CXX)
		string(REPLACE "-O2" "-O3" CMAKE_${lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}")
	endforeach()
endif()

# https://gitlab.kitware.com/cmake/cmake/issues/20256
if(APPLE)
	find_program(DSYMUTIL_PROGRAM dsymutil)
	if(DSYMUTIL_PROGRAM)
		foreach(lang C CXX)
			foreach(var LINK_EXECUTABLE CREATE_SHARED_LIBRARY)
				set(CMAKE_${lang}_${var} "${CMAKE_${lang}_${var}}" "${DSYMUTIL_PROGRAM} <TARGET>")
			endforeach()
		endforeach()
	endif()
endif()

function(sentry_install)
	if(SENTRY_ENABLE_INSTALL)
		install(${ARGN})
	endif()
endfunction()

# helper function to add sources to existing TARGET prepended with ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}
function(sentry_target_sources_cwd TARGET)
	cmake_parse_arguments(STSC "" "SUBDIR" "" ${ARGN})
	foreach(src ${STSC_UNPARSED_ARGUMENTS})
		if(IS_ABSOLUTE "${src}")
			target_sources(${TARGET} PRIVATE ${src})
		else()
			target_sources(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${STSC_SUBDIR}/${src}")
		endif()
	endforeach()
endfunction()

# ===== sentry library =====

add_library(sentry ${SENTRY_LIBRARY_TYPE} "${PROJECT_SOURCE_DIR}/vendor/mpack.c")
target_sources(sentry PRIVATE "${PROJECT_SOURCE_DIR}/include/sentry.h")
add_library(sentry::sentry ALIAS sentry)
add_subdirectory(src)

# we do not need this on android, only linux
if(LINUX)
	target_sources(sentry PRIVATE
		"${PROJECT_SOURCE_DIR}/vendor/stb_sprintf.c"
		"${PROJECT_SOURCE_DIR}/vendor/stb_sprintf.h"
	)
endif()

set_target_properties(sentry PROPERTIES PUBLIC_HEADER "include/sentry.h")

# check size type
include(CheckTypeSize)
check_type_size("long" CMAKE_SIZEOF_LONG)

# https://gitlab.kitware.com/cmake/cmake/issues/18393
if(SENTRY_BUILD_SHARED_LIBS)
	if(APPLE)
		sentry_install(FILES "$<TARGET_FILE:sentry>.dSYM" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
	elseif(MSVC)
		sentry_install(FILES "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:$<TARGET_PDB_FILE:sentry>>"
			DESTINATION "${CMAKE_INSTALL_BINDIR}")
	endif()
endif()

if(SENTRY_BUILD_SHARED_LIBS)
	target_compile_definitions(sentry PRIVATE SENTRY_BUILD_SHARED)
else()
	target_compile_definitions(sentry PUBLIC SENTRY_BUILD_STATIC)
endif()
target_compile_definitions(sentry PRIVATE SIZEOF_LONG=${CMAKE_SIZEOF_LONG})

if(SENTRY_TRANSPORT_CURL)
	find_package(CURL REQUIRED)
	if(TARGET CURL::libcurl)
		target_link_libraries(sentry PRIVATE CURL::libcurl)
	else()
		target_include_directories(sentry PRIVATE ${CURL_INCLUDE_DIR})
		# The exported sentry target must not contain any path of the build machine, therefore use generator expressions
		# FIXME: cmake 3.12 introduced the target CURL::libcurl
		string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_LIBRARIES "${CURL_LIBRARIES}")
		string(REPLACE ";" "$<SEMICOLON>" GENEX_CURL_COMPILE_DEFINITIONS "${CURL_COMPILE_DEFINITIONS}")
		target_link_libraries(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_LIBRARIES}>)
		target_compile_definitions(sentry PRIVATE $<BUILD_INTERFACE:${GENEX_CURL_COMPILE_DEFINITIONS}>)
	endif()
endif()

set_property(TARGET sentry PROPERTY C_VISIBILITY_PRESET hidden)
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CMAKE_ASM_MASM_FLAGS "${CMAKE_ASM_MASM_FLAGS} /safeseh")
	endif()

	# using `/Wall` is not feasible, as it spews tons of warnings from windows headers
	# supress C5105, introduced in VS 16.8, which breaks on the Windows SDKs own `winbase.h` header
	target_compile_options(sentry PRIVATE $<BUILD_INTERFACE:/W4 /wd5105>)
	# ignore all warnings for mpack
	set_source_files_properties(
		"${PROJECT_SOURCE_DIR}/vendor/mpack.c"
		PROPERTIES
		COMPILE_FLAGS
		"/W0"
	)

	# set static runtime if enabled
	if(SENTRY_BUILD_RUNTIMESTATIC)
		set_property(TARGET sentry PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
else()
	target_compile_options(sentry PRIVATE $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic>)
	# The crashpad and breakpad headers generate the following warnings that we
	# ignore specifically
	target_compile_options(sentry PRIVATE $<BUILD_INTERFACE:-Wno-variadic-macros -Wno-gnu-include-next -Wno-multichar>)
	# ignore all warnings for mpack
	set_source_files_properties(
		"${PROJECT_SOURCE_DIR}/vendor/mpack.c"
		PROPERTIES
		COMPILE_FLAGS
		"-w"
	)
endif()


target_include_directories(sentry
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

# The modulefinder and symbolizer need these two settings, and they are exported
# as `PUBLIC`, so libraries that depend on sentry get these too:
# `-E`: To have all symbols in the dynamic symbol table.
# `--build-id`: To have a build-id in the ELF object.
# FIXME: cmake 3.13 introduced target_link_options
option(SENTRY_EXPORT_SYMBOLS "Export symbols for modulefinder and symbolizer" ON)
if(SENTRY_EXPORT_SYMBOLS)
	target_link_libraries(sentry PUBLIC
		"$<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Android>>:-Wl,-E,--build-id=sha1>")
endif()

#respect CMAKE_SYSTEM_VERSION
if(WIN32)
	if(MSVC AND CMAKE_GENERATOR_TOOLSET MATCHES "_xp$")
		#force WINNT to 5.1 for Windows XP toolchain
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0501")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^10")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0A00")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.3")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0603")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.2")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0602")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.1")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0601")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^6.0")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0600")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^5.2")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0502")
	elseif(${CMAKE_SYSTEM_VERSION} MATCHES "^5.1")
		target_compile_definitions(sentry PRIVATE "_WIN32_WINNT=0x0501")
	endif()

	# crashpad does not support Windows XP toolset
	if(MSVC AND CMAKE_GENERATOR_TOOLSET MATCHES "_xp$" AND SENTRY_BACKEND_CRASHPAD)
		message(FATAL_ERROR "MSVC XP toolset does not support Crashpad")
	endif()
endif()

# handle platform libraries
if(ANDROID)
    set(_SENTRY_PLATFORM_LIBS "dl" "log")
elseif(LINUX)
    set(_SENTRY_PLATFORM_LIBS "dl" "rt")
elseif(WIN32)
    set(_SENTRY_PLATFORM_LIBS "dbghelp" "shlwapi" "version")
endif()

if(SENTRY_TRANSPORT_WINHTTP)
	list(APPEND _SENTRY_PLATFORM_LIBS "winhttp")
endif()

# handle platform threads library
if(SENTRY_LINK_PTHREAD)
	list(APPEND _SENTRY_PLATFORM_LIBS "Threads::Threads")
endif()

# apply platform libraries to sentry library
if(SENTRY_LIBRARY_TYPE STREQUAL "static")
    target_link_libraries(sentry PUBLIC ${_SENTRY_PLATFORM_LIBS})
else()
    target_link_libraries(sentry PRIVATE ${_SENTRY_PLATFORM_LIBS})
endif()

# suppress some errors and warnings for MinGW target
if(MINGW)
	target_compile_options(sentry PRIVATE
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-format
		-Wno-incompatible-pointer-types
		-Wno-incompatible-function-pointer-types
	)
endif()

if(SENTRY_WITH_LIBUNWINDSTACK)
	target_include_directories(sentry PRIVATE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/include>")
	add_subdirectory("${PROJECT_SOURCE_DIR}/external/libunwindstack-ndk/cmake")
	target_link_libraries(sentry PRIVATE unwindstack)
	if(NOT SENTRY_BUILD_SHARED_LIBS)
		sentry_install(TARGETS unwindstack EXPORT sentry
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
endif()

if(SENTRY_BACKEND_CRASHPAD)
	option(SENTRY_CRASHPAD_SYSTEM "Use system crashpad" OFF)
	if(SENTRY_CRASHPAD_SYSTEM)
		find_package(crashpad REQUIRED)
		target_link_libraries(sentry PUBLIC crashpad::client)
	else()
		# FIXME: required for cmake 3.12 and lower:
		# - NEW behavior lets normal variable override option
		cmake_policy(SET CMP0077 NEW)
		if(SENTRY_BUILD_SHARED_LIBS)
			set(CRASHPAD_ENABLE_INSTALL OFF CACHE BOOL "Enable crashpad installation" FORCE)
		else()
			set(CRASHPAD_ENABLE_INSTALL ON CACHE BOOL "Enable crashpad installation" FORCE)
		endif()
		add_subdirectory(external/crashpad crashpad_build)

		# set static runime if enabled
		if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
			set_property(TARGET crashpad_client PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_compat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_getopt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_handler PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_handler_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_minidump PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_snapshot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_tools PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_util PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET crashpad_zlib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
			set_property(TARGET mini_chromium PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		endif()

		target_link_libraries(sentry PRIVATE
			$<BUILD_INTERFACE:crashpad::client>
			$<INSTALL_INTERFACE:sentry_crashpad::client>
		)
		install(EXPORT crashpad_export NAMESPACE sentry_crashpad:: FILE sentry_crashpad-targets.cmake
			DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
		)
		if(WIN32 AND MSVC)
			sentry_install(FILES $<TARGET_PDB_FILE:crashpad_handler>
				DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
		endif()
	endif()
	add_dependencies(sentry crashpad::handler)
elseif(SENTRY_BACKEND_BREAKPAD)
	option(SENTRY_BREAKPAD_SYSTEM "Use system breakpad" OFF)
	if(SENTRY_BREAKPAD_SYSTEM)
		# system breakpad is using pkg-config, see `external/breakpad/breakpad-client.pc.in`
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(BREAKPAD REQUIRED IMPORTED_TARGET breakpad-client)
		target_link_libraries(sentry PUBLIC PkgConfig::BREAKPAD)
	else()
		add_subdirectory(external)
		target_include_directories(sentry PRIVATE
			"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/breakpad/src>"
		)
		target_link_libraries(sentry PRIVATE
			breakpad_client
		)
		if(NOT SENTRY_BUILD_SHARED_LIBS)
			sentry_install(TARGETS breakpad_client EXPORT sentry
				LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
			)
		endif()
	endif()
elseif(SENTRY_BACKEND_INPROC)
	target_compile_definitions(sentry PRIVATE SENTRY_WITH_INPROC_BACKEND)
endif()

option(SENTRY_INTEGRATION_QT "Build Qt integration")
if(SENTRY_INTEGRATION_QT)
	if(QT_DEFAULT_MAJOR_VERSION)
		# Let user choose major version
		set(Qt_VERSION_MAJOR ${QT_DEFAULT_MAJOR_VERSION})
	else()
		# Find best match, prioritizing Qt 6 if available
		find_package(Qt NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	endif()
	find_package(Qt${Qt_VERSION_MAJOR} COMPONENTS Core REQUIRED)
	message(STATUS "Found Qt: ${Qt${Qt_VERSION_MAJOR}_DIR} "
		"(found version \"${Qt${Qt_VERSION_MAJOR}_VERSION}\")")
	target_link_libraries(sentry PRIVATE Qt${Qt_VERSION_MAJOR}::Core)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(sentry-config.cmake.in sentry-config.cmake
	INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# generate package version file
# We would have liked to use `SameMinorVersion`, but that is only supported on
# CMake >= 3.11.
write_basic_package_version_file(sentry-config-version.cmake
	VERSION ${SENTRY_VERSION}
	COMPATIBILITY SameMajorVersion)

sentry_install(TARGETS sentry EXPORT sentry
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
sentry_install(EXPORT sentry NAMESPACE sentry:: FILE sentry-targets.cmake
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
sentry_install(
	FILES
		"${PROJECT_BINARY_DIR}/sentry-config.cmake"
		"${PROJECT_BINARY_DIR}/sentry-config-version.cmake"
	DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
if(WIN32 AND MSVC AND SENTRY_BUILD_SHARED_LIBS)
	sentry_install(FILES $<TARGET_PDB_FILE:sentry>
		DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
endif()

# ===== tests =====

if(SENTRY_BUILD_TESTS)
	add_subdirectory(tests/unit)
endif()

# ===== example, also used as integration test =====

if(SENTRY_BUILD_EXAMPLES)
	add_executable(sentry_example examples/example.c)
	target_link_libraries(sentry_example PRIVATE sentry)

	if(MSVC)
		target_compile_options(sentry_example PRIVATE $<BUILD_INTERFACE:/wd5105>)
	endif()

	# set static runtime if enabled
	if(SENTRY_BUILD_RUNTIMESTATIC AND MSVC)
		set_property(TARGET sentry_example PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()

	add_test(NAME sentry_example COMMAND sentry_example)
endif()
