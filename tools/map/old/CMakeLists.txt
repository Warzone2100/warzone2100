cmake_minimum_required (VERSION 2.8)

project (maptools)

# Options
option (STABS_DEBUG_SYMBOLS "Stabs debugging symbols" FALSE)
option (QT_IS_STATIC "Using static Qt" FALSE)

# Required warzone components
find_path(WZ_SOURCE_DIR lib/framework/wzglobal.h
        HINTS "${CMAKE_SOURCE_DIR}/../.."
        DOC "Warzone2100 source root directory path"
        )

find_path(WZ_BUILD_DIR config.h
        HINTS "${CMAKE_SOURCE_DIR}/../.."
        DOC "Path to directory containing the config.h file created by warzone's build configuration step")

# Uncomment when we actually need to link to the framework lib
#if (NOT WZ_FRAMEWORK_LIBRARY)
#    find_library(WZ_FRAMEWORK_LIBRARY name framework)
#endif ()
#set (WZ_FRAMEWORK_LIBRARY "${WZ_FRAMEWORK_LIBRARY}" CACHE FILEPATH "Path to Warzone's framework library")

#if (NOT WZ_FRAMEWORK_LIBRARY)
#    message (SEND_ERROR "Warzone's framework library not found")
#endif ()
if (NOT WZ_BUILD_DIR)
    message (SEND_ERROR "Warzone's config.h not found")
endif ()
if (NOT WZ_SOURCE_DIR)
    message (SEND_ERROR "Warzone's source dir not found")
else ()
    set (WZ_FRAMEWORK_INCLUDE_DIRS ${WZ_SOURCE_DIR} ${WZ_BUILD_DIR} CACHE INTERNAL "")
endif ()

if (NOT APPLE)
    add_definitions(-DHAVE_CONFIG_H)
endif ()

find_package (Qt4 REQUIRED)
include (${QT_USE_FILE})
add_definitions (${QT_DEFINITIONS})

if (QT_IS_STATIC)
    add_definitions ("-DQT_STATIC")
    add_definitions ("-DQT_NODLL")
    set (QT_CONFIG "static")

    # We need to use pkgconfig to get the linker flags for static qt
    find_package (PkgConfig)
    pkg_check_modules (_QT REQUIRED QtCore QtGui)
    set (QT_LIBRARIES ${_QT_STATIC_LIBRARIES})
endif ()

IF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")
    ADD_DEFINITIONS ("-DQT_NO_DEBUG")
ENDIF()

find_package (PNG 1.4 REQUIRED)
find_package (PhysFS 2 REQUIRED)

set (CMAKE_USE_PTHREADS_INIT TRUE)
find_package (Threads)

# set CMAKE_BUILD_TYPE to default
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
    # Set our own default flags at first run.
    if (NOT CONFIGURED)
        set (DEBUG_WARNINGS "-Wall -Wno-sign-compare")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")

        if (CMAKE_USE_PTHREADS_INIT)
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        endif ()

        set (CMAKE_CXX_FLAGS_DEBUG "-O1 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
        set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -DNDEBUG")
        set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
        set (CMAKE_CXX_FLAGS_PROFILE "-O2 -pg ${DEBUG_WARNINGS} -DNDEBUG")

        set (CMAKE_MODULE_LINKER_FLAGS_PROFILE "-pg")
        set (CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg")

        if (STABS_DEBUG_SYMBOLS)
            set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gstabs")
            set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstabs")
        else ()
            set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
            set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
        endif ()
    endif ()
endif ()

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

include_directories(${WZ_FRAMEWORK_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${QT_INCLUDES} ${PHYSFS_INCLUDE_DIR})

set(MAPTOOLS_LIBS ${PNG_LIBRARIES} ${PHYSFS_LIBRARY} ${QT_LIBRARIES})

# map2lnd
ADD_EXECUTABLE(map2lnd
                map2lnd.cpp
                maplib.cpp
                mapload.cpp)
TARGET_LINK_LIBRARIES(map2lnd ${MAPTOOLS_LIBS})

# map2png
ADD_EXECUTABLE(map2png
                map2png.cpp
                maplib.cpp
                mapload.cpp
                pngsave.cpp)
TARGET_LINK_LIBRARIES(map2png ${MAPTOOLS_LIBS})

# map2preview
ADD_EXECUTABLE(map2preview
                map2preview.cpp
                maplib.cpp
                mapload.cpp
                pngsave.cpp)
TARGET_LINK_LIBRARIES(map2preview ${MAPTOOLS_LIBS})

# mapconv
ADD_EXECUTABLE(mapconv
                mapconv.cpp
                maplib.cpp
                mapload.cpp)
TARGET_LINK_LIBRARIES(mapconv ${MAPTOOLS_LIBS})

# mapinfo
add_executable(mapinfo
               mapinfo.cpp
               mapload.cpp)
TARGET_LINK_LIBRARIES(mapinfo ${MAPTOOLS_LIBS})

## Other stuff
set (CONFIGURED YES CACHE INTERNAL "")

# vim: ts=4:sw=4:expandtab
