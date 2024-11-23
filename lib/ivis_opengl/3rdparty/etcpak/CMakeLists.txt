file(GLOB HEADERS "*.hpp")
file(GLOB SRC "*.cpp")

add_library(etcpak STATIC ${HEADERS} ${SRC})
set_property(TARGET etcpak PROPERTY FOLDER "3rdparty")
target_include_directories(etcpak INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_compile_definitions(etcpak PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

include(CheckCompilerFlagsOutput)
if(NOT MSVC)
	set(_supported_etcpak_cxx_compiler_flags "")

	# -Wshift-overflow		(GCC, Clang)
	check_compiler_flags_output("-Werror -Wno-shift-overflow -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-shift-overflow" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wlogical-not-parentheses		(Clang)
	check_compiler_flags_output("-Werror -Wno-logical-not-parentheses -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-logical-not-parentheses" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wcast-align				(GCC 3.4+, Clang 3.2+)
	check_compiler_flags_output("-Werror -Wno-cast-align -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-cast-align" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wmacro-redefined
	check_compiler_flags_output("-Werror -Wno-macro-redefined -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-macro-redefined" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wunused-function
	check_compiler_flags_output("-Werror -Wno-unused-function -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unused-function" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wunused-const-variable
	check_compiler_flags_output("-Werror -Wno-unused-const-variable -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-unused-const-variable" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wshadow
	check_compiler_flags_output("-Werror -Wno-shadow -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-shadow" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wconditional-uninitialized
	check_compiler_flags_output("-Werror -Wno-conditional-uninitialized -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-conditional-uninitialized" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	# -Wreturn-type
	check_compiler_flags_output("-Werror -Wno-return-type -Wno-error=cpp" COMPILER_TYPE CXX   OUTPUT_FLAGS "-Wno-return-type" OUTPUT_VARIABLE _supported_etcpak_cxx_compiler_flags APPEND)

	if (NOT _supported_etcpak_cxx_compiler_flags STREQUAL "")
		string(REPLACE " " ";" _supported_etcpak_cxx_compiler_flags "${_supported_etcpak_cxx_compiler_flags}")
		target_compile_options(etcpak PRIVATE ${_supported_etcpak_cxx_compiler_flags})
	endif()
endif()
