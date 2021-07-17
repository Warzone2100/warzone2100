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
