cmake_minimum_required (VERSION 3.5)
project (exception-handler CXX)

file(GLOB HEADERS "*.h")
set(SRC "dumpinfo.cpp" "exceptionhandler.cpp")

add_library(exception-handler STATIC ${HEADERS} ${SRC})
set_property(TARGET exception-handler PROPERTY FOLDER "lib")
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(exception-handler PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
		# Cross-compiling for Windows with MINGW
		target_sources(exception-handler PRIVATE "exchndl_mingw.cpp")
		target_link_libraries(exception-handler PRIVATE bfd iberty)
	else()
		# Compiling with MSVC (etc) on Windows
		target_sources(exception-handler PRIVATE "3rdparty/StackWalker.cpp" "exchndl_win.cpp")
		target_link_libraries(exception-handler PRIVATE dbghelp version shlwapi)
	endif()
endif()

target_link_libraries(exception-handler PRIVATE framework launchinfo)
