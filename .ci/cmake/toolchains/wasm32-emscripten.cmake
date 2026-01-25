# Toolchain for compiling for Emscripten

# Note:
# This is a bit of a hack.
# We actually want to use the Emscripten.cmake toolchain provided by Emscripten...
# However we also want to override and enable pthreads for everything

if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
   find_path(EMSCRIPTEN_ROOT "emcc")
else()
   set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
endif()

if(NOT EMSCRIPTEN_ROOT)
   if(NOT DEFINED ENV{EMSDK})
      message(FATAL_ERROR "The emcc compiler not found in PATH")
   endif()
   set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
endif()

if(NOT EXISTS "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
   message(FATAL_ERROR "Emscripten.cmake toolchain file not found")
endif()

include("${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")

# Always enable PThreads
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

# Enable optimizations for release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
