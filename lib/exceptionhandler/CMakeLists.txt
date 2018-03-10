cmake_minimum_required (VERSION 3.5)
project (exception-handler CXX)

file(GLOB HEADERS "*.h")
set(SRC "dumpinfo.cpp" "exceptionhandler.cpp")

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
	# Include exchndl.cpp when cross-compiling for Windows with MINGW
	list(APPEND SRC "exchndl.cpp")
endif()

add_library(exception-handler STATIC ${HEADERS} ${SRC})
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
  target_link_libraries(exception-handler PRIVATE bfd iberty)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  target_link_libraries(exception-handler PRIVATE dbghelp)
endif()
target_link_libraries(exception-handler PRIVATE framework)
