cmake_minimum_required(VERSION 3.5)
project(wz2100)

OPTION(ENABLE_NLS "Native Language Support" ON)
OPTION(WZ_PORTABLE "Portable (Windows-only)" ON)
OPTION(WZ_ENABLE_WARNINGS "Enable (additional) warnings" ON)
OPTION(WZ_ENABLE_WARNINGS_AS_ERRORS "Enable compiler flags that treat (most) warnings as errors" ON)

if(WZ_PORTABLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	message( WARNING "Portable build is only supported on Windows; Ignoring WZ_PORTABLE option" )
	unset(WZ_PORTABLE CACHE)
endif()

if(WZ_ENABLE_WARNINGS)
	set(WZ_Wno_ "-Wno-error=")
else()
	set(WZ_Wno_ "-Wno-")
endif()

if(MSVC)
	set(CMAKE_CXX_STANDARD 14)
else()
	set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Qt5 COMPONENTS Core Widgets Script Gui REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  find_package(PkgConfig)
  if(${CMAKE_CROSSCOMPILING})
    pkg_check_modules(QT5ALL REQUIRED Qt5Widgets Qt5Core Qt5Script)
    link_directories(${QT5ALL_LIBRARY_DIRS})
  endif()
endif()

# Use "-fPIC" / "-fPIE" for all targets by default, including static libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
INCLUDE(AddTargetLinkFlagsIfSupported)
CHECK_CXX_LINKER_FLAGS("${CMAKE_EXE_LINKER_FLAGS} -pie" LINK_FLAG_PIE_SUPPORTED)
if(LINK_FLAG_PIE_SUPPORTED AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

# Ensure all builds always have debug info built (MSVC)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" CACHE STRING "Flags used by the linker (Release builds)" FORCE)
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Enable stack protection, if supported by the compiler
# Prefer -fstack-protector-strong if supported, fall-back to -fstack-protector
check_c_compiler_flag(-fstack-protector-strong HAS_CFLAG_FSTACK_PROTECTOR_STRONG)
if (HAS_CFLAG_FSTACK_PROTECTOR_STRONG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
else()
	check_c_compiler_flag(-fstack-protector HAS_CFLAG_FSTACK_PROTECTOR)
	if (HAS_CFLAG_FSTACK_PROTECTOR)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
	endif()
endif()
check_cxx_compiler_flag(-fstack-protector-strong HAS_CXXFLAG_FSTACK_PROTECTOR_STRONG)
if (HAS_CXXFLAG_FSTACK_PROTECTOR_STRONG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
else()
	check_cxx_compiler_flag(-fstack-protector HAS_CXXFLAG_FSTACK_PROTECTOR)
	if (HAS_CXXFLAG_FSTACK_PROTECTOR)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
	endif()
endif()

include(CheckCompilerFlagsOutput)

# Configure compiler warnings for WZ
# NOTE: This should be called after the 3rdparty/ libs are added, so it only affects WZ code & frameworks
macro(CONFIGURE_WZ_COMPILER_WARNINGS)
	if(MSVC)
		# MSVC

		# Enable /W4
		# NOTE: Do not use /Wall, as it enables *many* warnings that are off by default. MS recommends /W4 (max)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

		# Enable some additional MSVC warnings (if compiling at warning level 3+)
		# C4204: nonstandard extension used : non-constant aggregate initializer
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34204")
		# C4512: 'class' : assignment operator could not be generated
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34512")

		# Enable some additional MSVC warnings (that are off by default)
		# C4191: unsafe conversion from 'type of expression' to 'type required'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34191")
		# C4263: 'function': member function does not override any base class virtual member function
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34263")
		# C4264: 'virtual_function': no override available for virtual member function from base 'class'; function is hidden
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34264")
		# C4265: 'class': class has virtual functions, but destructor is not virtual
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34265")
		# C4266: 'function' : no override available for virtual member function from base 'type'; function is hidden
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34266")
		# C4905: wide string literal cast to 'LPSTR'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14905")
		# C4906: string literal cast to 'LPWSTR'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14906")
		# C4928: illegal copy-initialization; more than one user-defined conversion has been implicitly applied
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14928")

		# C4289: nonstandard extension used : 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44289")
		# C4836: nonstandard extension used : 'type' : local types or unnamed types cannot be used as template arguments
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14836")

#		# NOTE: The following cannot currently be enabled because of Qt headers (at least Qt 5.6.x)
#		# C4946: reinterpret_cast used between related classes: 'class1' and 'class2'
#		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14946")

		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable /WX
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
		else()
			message( STATUS "WZ will *NOT* enable /WX" )
		endif()

		# Disable some warnings for WZ (permanently)
		# C4800: 'type' : forcing value to bool 'true' or 'false' (performance warning)
		# This warning is no longer generated in Visual Studio 2017+
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")

		# Disable some warnings for WZ (FIXME)
		# C4100: unreferenced formal parameter
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100")
		# C4456: declaration of 'identifier' hides previous local declaration
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456")
		# C4459: declaration of 'identifier' hides global declaration
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4459")
		# C4702: unreachable code
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702")
		# C4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4245")
		# C4701: Potentially uninitialized local variable 'name' used
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4701")

		# Qt headers (at least 5.6.x) necessitate disabling some warnings
		# C4127: conditional expression is constant
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")
	else()
		# GCC, Clang, etc
		# Comments are provided next to each warning option detailing expected compiler support (from GCC 3.4+, Clang 3.2+ - earlier versions may / may not support these options)

		set(_supported_c_compiler_flags "")
		set(_supported_cxx_compiler_flags "")

		# Enable -Wpedantic (if supported)
		check_compiler_flags_output("-Werror -Wpedantic -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wpedantic" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wpedantic -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wpedantic" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wall (if supported)
		check_compiler_flags_output("-Werror -Wall -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wall" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wall -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wall" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wextra (if supported)
		check_compiler_flags_output("-Werror -Wextra -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wextra" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wextra -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wextra" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-align				(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-qual				(GCC 3.4+, Clang 3.2+ (no-op until 3.6+))
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently conflicts with json.hpp
#		# -Wctor-dtor-privacy		(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0))
#		check_compiler_flags_output("-Werror -Wctor-dtor-privacy -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wctor-dtor-privacy" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wctor-dtor-privacy -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wctor-dtor-privacy" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Winit-self				(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0))
		check_compiler_flags_output("-Werror -Winit-self -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Winit-self" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Winit-self -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Winit-self" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently disabled
#		# -Wmissing-declarations	(GCC 3.4+, Clang 3.2+)
#		check_compiler_flags_output("-Werror -Wmissing-declarations -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-declarations" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wmissing-declarations -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-declarations" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently disabled because of miniupnpc (pending fix)
#		# -Wmissing-include-dirs	(GCC 4.0+, Clang 3.2+ (no-op through at least 6.0))
#		check_compiler_flags_output("-Werror -Wmissing-include-dirs -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-include-dirs" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wmissing-include-dirs -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-include-dirs" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnoexcept				(GCC 4.6+) [C++-only]
		check_compiler_flags_output("-Werror -Wnoexcept -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnoexcept" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Woverloaded-virtual		(GCC 3.4+, Clang 3.2+) [C++-only]
		check_compiler_flags_output("-Werror -Woverloaded-virtual -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Woverloaded-virtual" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wstrict-null-sentinel	(GCC 4.0+) [C++-only]
		check_compiler_flags_output("-Werror -Wstrict-null-sentinel -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wstrict-null-sentinel" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wwrite-strings			(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wwrite-strings -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wwrite-strings" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wwrite-strings -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wwrite-strings" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wpointer-arith			(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wpointer-arith -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wpointer-arith" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wpointer-arith -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wpointer-arith" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wstrict-prototypes		(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0)) [C-only]
		check_compiler_flags_output("-Werror -Wstrict-prototypes -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wstrict-prototypes" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)

#		# TODO: Enable this, and verify all code functions as expected
#		# -Wfloat-equal				(GCC 3.4+)
#		check_compiler_flags_output("-Werror -Wfloat-equal -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wfloat-equal" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wfloat-equal -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wfloat-equal" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wmissing-noreturn		(GCC 3.4+, Clang 3.3+)
		check_compiler_flags_output("-Werror -Wmissing-noreturn -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-noreturn" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wmissing-noreturn -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-noreturn" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# -Wundef					(GCC 3.4+, Clang (supported, but min version unclear))
#		check_compiler_flags_output("-Werror -Wundef -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wundef" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wundef -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wundef" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnon-virtual-dtor		(GCC 3.4+, Clang 3.2+) [C++ only]
		check_compiler_flags_output("-Werror -Wnon-virtual-dtor -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnon-virtual-dtor" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# FUTURE-TODO: Enable -Wshadow (lots of warnings to fix, some due to 3rdparty dependencies?)
#		# -Wshadow					(GCC 3.4+, Clang 3.2+)
#		check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# FUTURE-TODO: Enable -Wuseless-cast (large number of warnings to fix)
#		# -Wuseless-cast			(GCC 4.8+)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnull-dereference		(GCC 6.0+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wduplicated-cond			(GCC 6.0+)
		check_compiler_flags_output("-Werror -Wduplicated-cond -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wduplicated-cond" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wduplicated-cond -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wduplicated-cond" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Walloc-zero				(GCC 7.0+)
		check_compiler_flags_output("-Werror -Walloc-zero -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Walloc-zero" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Walloc-zero -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Walloc-zero" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Walloca-larger-than=1024	(GCC 7.0+)
		check_compiler_flags_output("-Werror -Walloca-larger-than=1024 -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Walloca-larger-than=1024" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Walloca-larger-than=1024 -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Walloca-larger-than=1024" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wrestrict				(GCC 7.0+)
		check_compiler_flags_output("-Werror -Wrestrict -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wrestrict" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wrestrict -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wrestrict" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable -Werror (if supported)
			check_compiler_flags_output("-Werror" COMPILER_TYPE C   OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
			check_compiler_flags_output("-Werror" COMPILER_TYPE CXX OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)
		else()
			message( STATUS "WZ will *NOT* enable -Werror" )
		endif()

		# Enable -Wlogical-op (if supported) (warning-only for now)
		check_compiler_flags_output("-Werror ${WZ_Wno_}logical-op -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}logical-op" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}logical-op -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}logical-op" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Handle "sign-compare" (warning-only for now)
		check_compiler_flags_output("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}sign-compare" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}sign-compare" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Handle "format-security" (warning-only for now)
		check_compiler_flags_output("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}format-security" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}format-security" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Disable -Wunused-parameter (FIXME)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		message( STATUS "Supported C compiler_flags=${_supported_c_compiler_flags}" )
		message( STATUS "Supported CXX compiler_flags=${_supported_cxx_compiler_flags}" )

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_supported_c_compiler_flags}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_supported_cxx_compiler_flags}")

	endif()
endmacro(CONFIGURE_WZ_COMPILER_WARNINGS)

add_definitions("-DHAVE_CONFIG_H")

# CMAKE_CURRENT_BINARY_DIR should come before the current source directory
# so that any build products are preferentially included over in-source build
# products that might have been generated by a different compilation method / run
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")
include_directories("3rdparty")
include_directories("3rdparty/glm")
include_directories("3rdparty/miniupnp")

SET(STDC_HEADERS ON)
SET(HAVE_CFPREFERENCESCOPYAPPVALUE OFF)
SET(_MINIX OFF)
SET(_POSIX_1_SOURCE OFF)
SET(_POSIX_SOURCE OFF)
SET(_XOPEN_SOURCE)
SET(PACKAGE "warzone2100")
SET(PACKAGE_BUGREPORT "http://wz2100.net/")
SET(PACKAGE_NAME "Warzone 2100")
SET(PACKAGE_TARNAME "warzone2100")
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  SET(_GNU_SOURCE ON)
endif()

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(alloca.h HAVE_ALLOCA_H)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES("sys/ucontext.h" HAVE_SYS_UCONTEXT_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)

INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(gettext HAVE_GETTEXT)
CHECK_FUNCTION_EXISTS(iconv HAVE_ICONV)
CHECK_FUNCTION_EXISTS(strlcat HAVE_SYSTEM_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_SYSTEM_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_VALID_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_VALID_STRLCPY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

SET(UPNPC_BUILD_STATIC ON CACHE BOOL "miniupnpc - Build static library" FORCE)
SET(UPNPC_BUILD_SHARED OFF CACHE BOOL "miniupnpc - Build shared library" FORCE)
SET(UPNPC_BUILD_TESTS OFF CACHE BOOL "miniupnpc - Build tests" FORCE)
SET(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "miniupnpc - Build samples" FORCE)

add_subdirectory(3rdparty/miniupnp/miniupnpc)
add_subdirectory(3rdparty/micro-ecc)
add_subdirectory(3rdparty/sha2)
add_subdirectory(3rdparty/fonts)
add_subdirectory(3rdparty/utf8proc)
CONFIGURE_WZ_COMPILER_WARNINGS()
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(icons)
add_subdirectory(pkg)

# Install base text / info files
set(ROOT_FILES ChangeLog AUTHORS COPYING.NONGPL COPYING COPYING.README README.md)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Target system is Windows
	# Must convert the ROOT_FILES to Windows line endings, and rename with ".txt" at the end
	set(_new_ROOT_FILES)
	foreach(rfile ${ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		# Read in the file
		file(READ ${rfile} _contents)
		# Strip all CRs
		string(REPLACE "\r" "" _contents ${_contents})
		# Convert all LFs to CRLFs
		string(REPLACE "\n" "\r\n" _contents ${_contents})
		# Write out the converted file
		set(_rfile_newfilename "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}.txt")
		file(WRITE "${_rfile_newfilename}" ${_contents})
		list(APPEND _new_ROOT_FILES "${_rfile_newfilename}")
	endforeach()
	set(ROOT_FILES ${_new_ROOT_FILES})
else()
	# Just copy the files to the build directory
	foreach(rfile ${ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		configure_file(${rfile} "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}" COPYONLY)
	endforeach()
endif()
foreach(rfile ${ROOT_FILES})
	install(FILES ${rfile}
			COMPONENT Core
			DESTINATION "."
	)
endforeach()

# Add "dist" target alias (using CPack package_source)
get_filename_component(_cmake_path ${CMAKE_COMMAND} PATH)
find_program(CPACK_COMMAND cpack ${_cmake_path})
unset(_cmake_path)
if(CPACK_COMMAND)
	add_custom_target(dist
		COMMAND ${CPACK_COMMAND} --config ${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endif()
