CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(maptools)

# compile in debug mode
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: None Debug Debug2 Debug3 Release RelWithDebInfo MinSizeRel."
      FORCE)
endif ()

find_package(PhysFS 2 REQUIRED)

find_package(PNG REQUIRED)

set(QT_USE_IMPORTED_TARGETS TRUE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# Ugly OS-X Fix, should find a better way.
IF(${CMAKE_SYSTEM_NAME} MATCHES "^Darwin")
	SET(QT_QTCORE_LIBRARY "-framework QtCore")
ENDIF()

IF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")
    ADD_DEFINITIONS("-DQT_NO_DEBUG")
ENDIF()

set(DEBUG_WARNINGS "-Wall -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IEEE_FP_FLAG} -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
add_definitions(-DSTREFLOP_SSE)
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()
set(CMAKE_CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG_ADDITIONAL} -O1 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG2 "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG3 "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE "-O2 -pg ${DEBUG_WARNINGS} -DNDEBUG")
set(CMAKE_MODULE_LINKER_FLAGS_PROFILE "-pg")
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg")
if (STABS_DEBUG_SYMBOLS)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gstabs")
    set(CMAKE_CXX_FLAGS_DEBUG2 "${CMAKE_CXX_FLAGS_DEBUG2} -gstabs")
    set(CMAKE_CXX_FLAGS_DEBUG3 "${CMAKE_CXX_FLAGS_DEBUG3} -gstabs")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstabs")
ELSE ()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb1")
    set(CMAKE_CXX_FLAGS_DEBUG2 "${CMAKE_CXX_FLAGS_DEBUG2} -ggdb2")
    set(CMAKE_CXX_FLAGS_DEBUG3 "${CMAKE_CXX_FLAGS_DEBUG3} -ggdb3")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
ENDIF ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/../.." ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${QT_INCLUDES} ${PHYSFS_INCLUDE_DIR})

set(MAPTOOLS_LIBS ${PNG_LIBRARIES} ${PHYSFS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})

# map2lnd
# ADD_EXECUTABLE(map2lnd
#                map2lnd.cpp
#                mapload.cpp)
# TARGET_LINK_LIBRARIES(map2lnd ${MAPTOOLS_LIBS})

# map2png
# ADD_EXECUTABLE(map2png
#                map2png.cpp
#                mapload.cpp
#                pngsave.cpp)
# TARGET_LINK_LIBRARIES(map2png ${MAPTOOLS_LIBS})

# map2preview
ADD_EXECUTABLE(map2preview
               map2preview.cpp
               maplib.cpp
               mapload.cpp
               pngsave.cpp
               wzconfig.cpp)
TARGET_LINK_LIBRARIES(map2preview ${MAPTOOLS_LIBS})

# map2png
ADD_EXECUTABLE(mapconv
               mapconv.cpp
               maplib.cpp
               mapload.cpp
               pngsave.cpp)
TARGET_LINK_LIBRARIES(mapconv ${MAPTOOLS_LIBS})

