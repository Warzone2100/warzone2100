cmake_minimum_required (VERSION 3.5)
project (exception-handler CXX)

file(GLOB HEADERS "*.h")
set(SRC "dumpinfo.cpp" "exceptionhandler.cpp")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
		# Cross-compiling for Windows with MINGW
		list(APPEND SRC "exchndl_mingw.cpp")
	else()
		# Compiling with MSVC (etc) on Windows
		list(APPEND SRC "3rdparty/StackWalker.cpp")
		list(APPEND SRC "exchndl_win.cpp")
	endif()
endif()

add_library(exception-handler STATIC ${HEADERS} ${SRC})
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
  target_link_libraries(exception-handler PRIVATE bfd iberty)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  target_link_libraries(exception-handler PRIVATE dbghelp version shlwapi)
endif()
target_link_libraries(exception-handler PRIVATE framework)
