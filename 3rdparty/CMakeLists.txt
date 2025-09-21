cmake_minimum_required (VERSION 3.16...3.31)

include(CheckCompilerFlagsOutput)

SET(UTF8PROC_INSTALL OFF CACHE BOOL "Enable installation of utf8proc" FORCE)
add_subdirectory(utf8proc EXCLUDE_FROM_ALL)
set_property(TARGET utf8proc PROPERTY FOLDER "3rdparty")
if(NOT MSVC)
	set(_supported_utf8proc_cxx_compiler_flags "")

	# -Wassign-enum
	check_compiler_flags_output("-Werror -Wno-assign-enum -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-assign-enum" OUTPUT_VARIABLE _supported_utf8proc_cxx_compiler_flags APPEND)

	if (NOT _supported_utf8proc_cxx_compiler_flags STREQUAL "")
		string(REPLACE " " ";" _supported_utf8proc_cxx_compiler_flags "${_supported_utf8proc_cxx_compiler_flags}")
		target_compile_options(utf8proc PRIVATE ${_supported_utf8proc_cxx_compiler_flags})
	endif()
endif()

add_subdirectory(launchinfo EXCLUDE_FROM_ALL)
set_property(TARGET launchinfo PROPERTY FOLDER "3rdparty")

SET(FMT_INSTALL OFF CACHE BOOL "Generate the install target" FORCE)
SET(FMT_SYSTEM_HEADERS ON CACHE BOOL "Expose headers with marking them as system." FORCE)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
set_property(TARGET fmt PROPERTY FOLDER "3rdparty")
# Define a custom inline namespace for the fmt used by WZ, to prevent ODR violations if static linking to any libraries that themselves embed (a different version of) fmt
target_compile_definitions(fmt PUBLIC "FMT_BEGIN_NAMESPACE=namespace fmt { inline namespace v11_wz2100 {")
target_compile_definitions(fmt PUBLIC "FMT_END_NAMESPACE=}}")


# inih library
add_library(inih STATIC "inih/ini.h" "inih/ini.c")
set_property(TARGET inih PROPERTY FOLDER "3rdparty")
target_include_directories(inih PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(inih PRIVATE
	"-DINI_API="
	"-DINI_ALLOW_MULTILINE=0"
	"-DINI_ALLOW_BOM=1"
	"-DINI_ALLOW_INLINE_COMMENTS=0"
	"-DINI_MAX_LINE=1024"
	"-DINI_ALLOW_REALLOC=1"
	"-DINI_INITIAL_ALLOC=1024"
)

# re2
SET(RE2_BUILD_TESTING OFF CACHE BOOL "enable testing for RE2" FORCE)
add_subdirectory(re2 EXCLUDE_FROM_ALL)
target_include_directories(re2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/re2>
)
set_property(TARGET re2 PROPERTY FOLDER "3rdparty")
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS NO)	# -Wmissing-field-initializers
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_GCC_WARN_SHADOW NO)	# -Wshadow
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-missing-field-initializers -Wno-shadow")

add_subdirectory(EmbeddedJSONSignature EXCLUDE_FROM_ALL)
set_property(TARGET EmbeddedJSONSignature PROPERTY FOLDER "3rdparty")

if(ENABLE_DISCORD)
	add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
	set_property(TARGET discord-rpc PROPERTY FOLDER "3rdparty")
endif()

find_package(SQLite3 3.14 REQUIRED)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)
set(SQLITE_HAS_CODEC OFF CACHE BOOL "Enable database encryption API. Not available in the public release of SQLite." FORCE)
if(SQLite3_VERSION VERSION_LESS 3.19)
	set(SQLITE_USE_LEGACY_STRUCT ON CACHE BOOL "Fallback to forward declaration of legacy struct sqlite3_value (pre SQLite 3.19)" FORCE)
endif()
set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "Add the internal SQLite3 source to the project." FORCE)
set(SQLITECPP_INCLUDE_SCRIPT OFF CACHE BOOL "Include config & script files." FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Run cpplint.py tool for Google C++ StyleGuide." FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "Run cppcheck C++ static analysis tool." FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "Build examples." FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "Build and run tests." FORCE)
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(SQLITECPP_USE_STACK_PROTECTION OFF CACHE BOOL "USE Stack Protection hardening." FORCE)
endif()
add_subdirectory(SQLiteCpp EXCLUDE_FROM_ALL)
set_property(TARGET SQLiteCpp PROPERTY FOLDER "3rdparty")

# glad library

set(GLAD_HEADERS
	glad/include/glad/glad.h
	glad/include/KHR/khrplatform.h
)

set(GLAD_SOURCES
	glad/src/glad.c
)

add_library(glad STATIC ${GLAD_HEADERS} ${GLAD_SOURCES})
set_property(TARGET glad PROPERTY FOLDER "3rdparty")
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

set(JSON_ImplicitConversions OFF CACHE BOOL "Enable implicit conversions." FORCE)
set(JSON_SystemInclude ON CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)
add_subdirectory(json EXCLUDE_FROM_ALL)
# Temporary workaround until we use the new NLOHMANN_JSON_NAMESPACE (etc) macros
target_compile_definitions(nlohmann_json INTERFACE "NLOHMANN_JSON_NAMESPACE=nlohmann;NLOHMANN_JSON_NAMESPACE_BEGIN=namespace nlohmann {;NLOHMANN_JSON_NAMESPACE_END=}")

add_subdirectory(optional-lite EXCLUDE_FROM_ALL)

add_subdirectory(quickjs-wz EXCLUDE_FROM_ALL)
set_property(TARGET qjs PROPERTY FOLDER "3rdparty")


if (WZ_ENABLE_BASIS_UNIVERSAL AND NOT WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)

	# basis-universal
	set(_ORIGINAL_CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}")
	if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_HOST_TRIPLET)
	  # Partial workaround for: https://github.com/microsoft/vcpkg/issues/17001
	  # Explicitly add the HOST_TRIPLET paths to CMAKE_PROGRAM_PATH
	  list(APPEND CMAKE_PROGRAM_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}")
	endif()
	find_program(BASIS_UNIVERSAL_CLI NAMES basisu PATH_SUFFIXES "tools/basisu" NO_CACHE)
	set(CMAKE_PROGRAM_PATH "${_ORIGINAL_CMAKE_PROGRAM_PATH}")
	if(NOT BASIS_UNIVERSAL_CLI)
	  message(STATUS "Pre-installed basisu tool not found - attempting to build for host system")

	  # Build the tool for the host system at configure time
	  set(BASISU_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/basis_universal_host_tool/")
	  execute_process(
		  COMMAND ${CMAKE_COMMAND} -E make_directory "${BASISU_BINARY_DIR}"
		  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal_host_build"
		  RESULT_VARIABLE _basis_result
	  )
	  if(NOT _basis_result EQUAL 0)
		message(FATAL_ERROR "Failed to create directory for basis_universal_host_build")
	  endif()
	  execute_process(
		  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal_host_build"
		  WORKING_DIRECTORY "${BASISU_BINARY_DIR}"
		  RESULT_VARIABLE _basis_result
	  )
	  if(NOT _basis_result EQUAL 0)
		message(FATAL_ERROR "Failed to configure basis_universal_host_build")
	  endif()
	  execute_process(
		  COMMAND ${CMAKE_COMMAND} --build "${BASISU_BINARY_DIR}" --config Release
		  WORKING_DIRECTORY "${BASISU_BINARY_DIR}"
		  RESULT_VARIABLE _basis_result
	  )
	  if(NOT _basis_result EQUAL 0)
		message(FATAL_ERROR "Basis-Universal host tool build failed")
	  endif()
	  message(STATUS "Basis-Universal Host Tool Built")

	  find_program(BASIS_UNIVERSAL_CLI NAMES basisu PATHS "${BASISU_BINARY_DIR}/basis_install/bin" NO_CACHE NO_DEFAULT_PATH)
	  if(NOT BASIS_UNIVERSAL_CLI)
		message(FATAL_ERROR "Built basisu CLI tool not found!")
	  endif()
	else()
	  message(STATUS "Pre-installed basisu tool found: ${BASIS_UNIVERSAL_CLI}")
	endif()

	# Test basisu -version
	execute_process(
		COMMAND "${BASIS_UNIVERSAL_CLI}" -version
		OUTPUT_VARIABLE BASISU_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE _basisu_result
	)
	if(_basisu_result AND NOT _basisu_result EQUAL 0)
		message(FATAL_ERROR "basisu does not seem to be executable on this host?: ${BASIS_UNIVERSAL_CLI} (exit code: ${_basisu_result})")
	endif()
	message(STATUS "Found basisu: ${BASIS_UNIVERSAL_CLI} ${BASISU_VERSION}")

	set(BASIS_UNIVERSAL_CLI "${BASIS_UNIVERSAL_CLI}" PARENT_SCOPE)

endif()

if (WZ_ENABLE_BASIS_UNIVERSAL)

	# basis-universal transcoder

	add_library(basis_transcoder STATIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.h" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/zstd/zstddeclib.c")
	set_property(TARGET basis_transcoder PROPERTY FOLDER "3rdparty")
	target_include_directories(basis_transcoder SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder")

	if(NOT MSVC)
		set(_supported_basistranscoder_cxx_compiler_flags "")

		# -fno-strict-aliasing		(GCC, Clang)
		check_compiler_flags_output("-Werror -fno-strict-aliasing -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-fno-strict-aliasing" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wcast-align				(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wno-cast-align -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-cast-align" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wdeprecated-declarations
		check_compiler_flags_output("-Werror -Wno-deprecated-declarations -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-deprecated-declarations" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wunused-but-set-variable
		check_compiler_flags_output("-Werror -Wno-unused-but-set-variable -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unused-but-set-variable" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wunused-function
		check_compiler_flags_output("-Werror -Wno-unused-function -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unused-function" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wunused-const-variable
		check_compiler_flags_output("-Werror -Wno-unused-const-variable -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unused-const-variable" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wunknown-warning-option
		check_compiler_flags_output("-Werror -Wno-unknown-warning-option -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unknown-warning-option" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		# -Wnontrivial-memcall
		check_compiler_flags_output("-Werror -Wno-nontrivial-memcall -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-nontrivial-memcall" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

		if (NOT _supported_basistranscoder_cxx_compiler_flags STREQUAL "")
			string(REPLACE " " ";" _supported_basistranscoder_cxx_compiler_flags "${_supported_basistranscoder_cxx_compiler_flags}")
			target_compile_options(basis_transcoder PRIVATE ${_supported_basistranscoder_cxx_compiler_flags})
		endif()
	endif()

	# Must also set BASISD_IS_BIG_ENDIAN if the target platform is big-endian!
	if (NOT DEFINED CMAKE_CXX_BYTE_ORDER AND CMAKE_VERSION VERSION_LESS 3.20)
		# CMake < 3.20 does not have CMAKE_<LANG>_BYTE_ORDER
		# Instead, use the older TestBigEndian module (although this may not work for cross-compilation)
		if (NOT CMAKE_CROSSCOMPILING)
			include(TestBigEndian)
			test_big_endian(IS_BIGENDIAN)
			if (IS_BIGENDIAN)
				set(CMAKE_CXX_BYTE_ORDER "BIG_ENDIAN")
			endif()
		else()
			message(WARNING "Unable to determine endianness for target architecture. Either upgrade to CMake 3.20+, or - if big endian - manually set the CMAKE_CXX_BYTE_ORDER cache variable to \"BIG_ENDIAN\". Otherwise, assuming little endian.")
		endif()
	endif()
	if (DEFINED CMAKE_CXX_BYTE_ORDER AND CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
		message(STATUS "Defining BASISD_IS_BIG_ENDIAN=1")
		target_compile_definitions(basis_transcoder PRIVATE "-DBASISD_IS_BIG_ENDIAN=1")
	endif()

	# Must set BASISU_HAVE_STD_TRIVIALLY_COPYABLE if the target supports std::is_trivially_copyable
	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("
		#include <type_traits>
		const bool val = std::is_trivially_copyable<bool>::value;
		int main()
		{
			return 0;
		}"
		HAVE_STD_IS_TRIVIALLY_COPYABLE
	)
	if (HAVE_STD_IS_TRIVIALLY_COPYABLE)
		target_compile_definitions(basis_transcoder PUBLIC "-DBASISU_HAVE_STD_TRIVIALLY_COPYABLE")
	endif()

	# Disable certain transcoder formats
	target_compile_definitions(basis_transcoder PRIVATE "-DBASISD_SUPPORT_ATC=0" "-DBASISD_SUPPORT_PVRTC1=0" "-DBASISD_SUPPORT_PVRTC2=0")

endif(WZ_ENABLE_BASIS_UNIVERSAL)

if (WZ_PROFILING_NVTX)
	include(FetchContent)
	FetchContent_Declare(
	  nvtx
	  GIT_REPOSITORY https://github.com/NVIDIA/NVTX.git
	  GIT_TAG        a1ceb0677f67371ed29a2b1c022794f077db5fe7
	)

	FetchContent_MakeAvailable(nvtx)
	set(PROFILING_NVTX_INCLUDE ${CUDAToolkit_INCLUDE_DIRS} PARENT_SCOPE)

endif ()

set(PLUM_NO_EXAMPLE ON CACHE BOOL "Disable example build" FORCE)
add_subdirectory(libplum EXCLUDE_FROM_ALL)
set_target_properties(plum plum-static PROPERTIES FOLDER "3rdparty")
if(NOT MSVC)
	set(_supported_libplum_c_compiler_flags "")

	# -Wshadow
	check_compiler_flags_output("-Werror -Wno-shadow -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-shadow" OUTPUT_VARIABLE _supported_libplum_c_compiler_flags APPEND)

	if (NOT _supported_libplum_c_compiler_flags STREQUAL "")
		string(REPLACE " " ";" _supported_libplum_c_compiler_flags "${_supported_libplum_c_compiler_flags}")
		target_compile_options(plum PRIVATE ${_supported_libplum_c_compiler_flags})
		target_compile_options(plum-static PRIVATE ${_supported_libplum_c_compiler_flags})
	endif()
endif()

set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(expected EXCLUDE_FROM_ALL)
# There isn't any release note or established CMake policy about this behavior,
# but looks like prior to CMake 3.19 only a handful of `INTERFACE_*` properties
# were allowed for INTERFACE libraries (which is our case). This restriction
# seems to be lifted in CMake 3.19 and later.
#
# See https://discourse.cmake.org/t/how-to-find-current-interface-library-property-whitelist/4784/2
# for some clarification about this.
if (CMAKE_VERSION VERSION_GREATER "3.18")
	set_target_properties(expected PROPERTIES FOLDER "3rdparty")
endif()

set(_default_enable_gns_network_backend_value ON)
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(_default_enable_gns_network_backend_value OFF)
endif()

option(ENABLE_GNS_NETWORK_BACKEND "Enable building Warzone 2100 with GameNetworkingSockets network backend support" ${_default_enable_gns_network_backend_value})
if (ENABLE_GNS_NETWORK_BACKEND)
	message(STATUS "GameNetworkingSockets backend support: ENABLED")
	# This will be propagated to the autogenerated wz-specific config header.
	set(WZ_GNS_NETWORK_BACKEND_ENABLED ON CACHE INTERNAL "Enables Valve GNS network backend support." FORCE)
	# Force usage of libsodium as the crypto backend.
	set(USE_CRYPTO libsodium CACHE INTERNAL "Crypto library to use for AES/SHA256" FORCE)
	# Don't build the shared variant of the client library: we don't use it anyway, plus it causes build conflicts for macOS.
	set(BUILD_SHARED_LIB OFF CACHE INTERNAL "Build the shared library version of the client library" FORCE)
	add_subdirectory(GameNetworkingSockets EXCLUDE_FROM_ALL)
	# Avoid absl symbol leaks with protobuf>=22.0.0 by always passing -DNDEBUG
	if(TARGET GameNetworkingSockets_s)
		target_compile_options(GameNetworkingSockets_s PRIVATE -DNDEBUG)
	endif()
	unset(BUILD_SHARED_LIB CACHE)
else()
	message(STATUS "GameNetworkingSockets backend support: DISABLED")
endif()
