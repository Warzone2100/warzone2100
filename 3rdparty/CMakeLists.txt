cmake_minimum_required (VERSION 3.5)

SET(UTF8PROC_INSTALL OFF CACHE BOOL "Enable installation of utf8proc" FORCE)
add_subdirectory(utf8proc)
set_property(TARGET utf8proc PROPERTY FOLDER "3rdparty")

add_subdirectory(launchinfo)
set_property(TARGET launchinfo PROPERTY FOLDER "3rdparty")

add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "3rdparty")

SET(RE2_BUILD_TESTING OFF CACHE BOOL "enable testing for RE2" FORCE)
add_subdirectory(re2 EXCLUDE_FROM_ALL)
target_include_directories(re2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/re2>
)
set_property(TARGET re2 PROPERTY FOLDER "3rdparty")
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS NO)	# -Wmissing-field-initializers
set_property(TARGET re2 PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-missing-field-initializers")

add_subdirectory(EmbeddedJSONSignature)
set_property(TARGET EmbeddedJSONSignature PROPERTY FOLDER "3rdparty")

if(ENABLE_DISCORD)
	add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
	set_property(TARGET discord-rpc PROPERTY FOLDER "3rdparty")
endif()

find_package(SQLite3 3.14 REQUIRED)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)
set(SQLITE_ENABLE_COLUMN_METADATA OFF CACHE BOOL "Enable Column::getColumnOriginName(). Require support from sqlite3 library." FORCE)
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
set_property(TARGET quickjs PROPERTY FOLDER "3rdparty")


if (WZ_ENABLE_BASIS_UNIVERSAL AND NOT WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)

	# basis-universal
	set(_ORIGINAL_CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}")
	if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_HOST_TRIPLET)
	  # Partial workaround for: https://github.com/microsoft/vcpkg/issues/17001
	  # Explicitly add the HOST_TRIPLET paths to CMAKE_PROGRAM_PATH
	  list(APPEND CMAKE_PROGRAM_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools")
	  file(GLOB Z_VCPKG_TOOLS_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools/*")
	  foreach(Z_VCPKG_TOOLS_DIR IN LISTS Z_VCPKG_TOOLS_DIRS)
		if(IS_DIRECTORY "${Z_VCPKG_TOOLS_DIR}")
		  list(APPEND CMAKE_PROGRAM_PATH "${Z_VCPKG_TOOLS_DIR}")
		endif()
	  endforeach()
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

	set(BASIS_UNIVERSAL_CLI "${BASIS_UNIVERSAL_CLI}" PARENT_SCOPE)

endif()

if (WZ_ENABLE_BASIS_UNIVERSAL)

	# basis-universal transcoder

	add_library(basis_transcoder STATIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder/basisu_transcoder.h" "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/zstd/zstddeclib.c")
	set_property(TARGET basis_transcoder PROPERTY FOLDER "3rdparty")
	target_include_directories(basis_transcoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder")

	include(CheckCompilerFlagsOutput)
	if(NOT MSVC)
		set(_supported_basistranscoder_cxx_compiler_flags "")

		# -fno-strict-aliasing		(GCC, Clang)
		check_compiler_flags_output("-Werror -fno-strict-aliasing -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-fno-strict-aliasing" OUTPUT_VARIABLE _supported_basistranscoder_cxx_compiler_flags APPEND)

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
