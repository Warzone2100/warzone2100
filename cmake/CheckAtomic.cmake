# Derived from: https://github.com/llvm/llvm-project/blob/main/llvm/cmake/modules/CheckAtomic.cmake
# License: https://github.com/llvm/llvm-project/blob/main/llvm/LICENSE.TXT

cmake_minimum_required(VERSION 3.16...3.31)

INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckLibraryExists)

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

function(check_working_cxx_atomics varname)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
  CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
std::atomic<int> x;
std::atomic<short> y;
std::atomic<char> z;
int main() {
  ++z;
  ++y;
  return ++x;
}
" ${varname})
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics)

# Check for (non-64-bit) atomic operations.
if(MSVC)
  set(HAVE_CXX_ATOMICS_WITHOUT_LIB TRUE)
elseif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # First check if atomics work without the library.
  check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITHOUT_LIB)
  # If not, check if the library exists, and atomics work with it.
  if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
    check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
    if(HAVE_LIBATOMIC)
      list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
      check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITH_LIB)
      if (NOT HAVE_CXX_ATOMICS_WITH_LIB)
        message(FATAL_ERROR "Host compiler must support std::atomic!")
      endif()
    else()
      message(FATAL_ERROR "Host compiler appears to require libatomic, but cannot find it.")
    endif()
  endif()
endif()

function(TARGET_LINK_ATOMICS_IF_NEEDED target)
  if(HAVE_CXX_ATOMICS_WITH_LIB)
	message(STATUS "Linking atomic library")
	target_link_libraries(${target} atomic)
  elseif(HAVE_CXX_ATOMICS_WITHOUT_LIB)
	message(STATUS "Atomics support built-in")
  endif()
endfunction(TARGET_LINK_ATOMICS_IF_NEEDED)
