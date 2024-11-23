set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH)

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

# Get the path to *this* triplet file, and then back up to get the known path to the meta-toolchain in WZ's repo
get_filename_component(WZ_WASM_META_TOOLCHAIN "${CMAKE_CURRENT_LIST_DIR}/../../cmake/toolchains/wasm32-emscripten.cmake" ABSOLUTE)
if(NOT EXISTS "${WZ_WASM_META_TOOLCHAIN}")
   message(FATAL_ERROR "Failed to find WZ's toolchains/wasm32-emscripten.cmake")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${WZ_WASM_META_TOOLCHAIN}")

set(VCPKG_BUILD_TYPE "release")
