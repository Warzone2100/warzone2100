SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_C_COMPILER   i586-mingw32msvc-gcc)

SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)

# Define paths to search for libraries
SET(CMAKE_FIND_ROOT_PATH /usr/i586-mingw32msvc/ ${CMAKE_SOURCE_DIR}/win32/build/libs/)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Don't search in native paths, just the specified root paths
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# FIXME CMake's defaults struggle to find these
SET(OPENAL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/win32/build/libs/include/AL)
SET(OPENAL_LIBRARY ${CMAKE_SOURCE_DIR}/win32/build/libs/lib/libopenal32.a)
# Our openal lib doesn't have correct capitalisation (e.g. OpenAL or al or openal or OpenAL32) , which causes issues.

SET(SDL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/win32/build/libs/include/SDL)

SET(NEED_DEP_DEPS TRUE) # Our CC dev pkg don't link the deps of the deps, so we need to do this

SET(MINGW TRUE) # just in case...
