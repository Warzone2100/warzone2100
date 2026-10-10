###############################################
# WZ process hardening & filesystem confinement

file(GLOB HEADERS "*.h")
set(SRC "sandbox_common.cpp")

add_library(sandbox STATIC ${HEADERS} ${SRC})
set_property(TARGET sandbox PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(sandbox)

# The syscall filter needs libseccomp, and the rest of the backend still builds and runs without it.
# >= 2.5.0 for SCMP_FLTATR_CTL_SSB (needed on a kernel before 5.16 to leave speculative store bypass alone).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBSECCOMP QUIET "libseccomp>=2.5.0")
	endif()
	if(LIBSECCOMP_FOUND)
		target_compile_definitions(sandbox PRIVATE "WZ_SANDBOX_HAVE_LIBSECCOMP")
		target_include_directories(sandbox PRIVATE ${LIBSECCOMP_INCLUDE_DIRS})
		target_link_libraries(sandbox PRIVATE ${LIBSECCOMP_LIBRARIES})
	else()
		message(STATUS "libseccomp >= 2.5.0 not found - building without the syscall filter")
	endif()
endif()

# An in-process sampler needs perf_event_open, which the syscall filter otherwise denies
if(WZ_PROFILING_INSTRUMENTATION OR WZ_PROFILING_NVTX OR WZ_PROFILING_VTUNE)
	target_compile_definitions(sandbox PRIVATE "WZ_SANDBOX_ALLOW_PERF_EVENTS")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_sources(sandbox PRIVATE "sandbox_win.cpp")
elseif(APPLE)
	target_sources(sandbox PRIVATE "sandbox_mac.cpp")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_sources(sandbox PRIVATE "sandbox_linux.cpp")
else()
	target_sources(sandbox PRIVATE "sandbox_null.cpp")
endif()
