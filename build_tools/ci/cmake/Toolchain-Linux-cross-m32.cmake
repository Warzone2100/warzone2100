# Toolchain for cross-compiling i686 (32-bit) on 64-bit Linux

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" CACHE STRING "C compiler flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "C++ compiler flags" FORCE)

set(LIB32 "/usr/lib") # Fedora

set(CMAKE_FIND_ROOT_PATH "${LIB32}" CACHE STRING "Find root path" FORCE)

#set(CMAKE_LIBRARY_PATH "${LIB32}" CACHE STRING "Library search path" FORCE)

set(CMAKE_EXE_LINKER_FLAGS "-m32 -L${LIB32}" CACHE STRING "Executable linker flags" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "-m32 -L${LIB32}" CACHE STRING "Shared library linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "-m32 -L${LIB32}" CACHE STRING "Module linker flags" FORCE)

# Point pkgconfig at 32-bit .pc files first, falling back to regular system .pc files
if(EXISTS "${LIB32}/pkgconfig")
	set(ENV{PKG_CONFIG_LIBDIR} "${LIB32}/pkgconfig:/usr/share/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
