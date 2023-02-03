cmake_minimum_required(VERSION 3.5...3.24)

set(_OLD_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(WZVcpkgInit) # Must come before project() command
set(CMAKE_MODULE_PATH "${_OLD_CMAKE_MODULE_PATH}")
unset(_OLD_CMAKE_MODULE_PATH)

project(warzone2100)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

include(CMakeDependentOption)

OPTION(ENABLE_DOCS "Enable documentation generation" ON)
OPTION(ENABLE_NLS "Native Language Support" ON)
OPTION(WZ_ENABLE_WARNINGS "Enable (additional) warnings" OFF)
OPTION(WZ_ENABLE_WARNINGS_AS_ERRORS "Enable compiler flags that treat (most) warnings as errors" ON)
OPTION(WZ_ENABLE_BACKEND_VULKAN "Enable Vulkan backend" ON)
OPTION(WZ_ENABLE_BASIS_UNIVERSAL "Enable Basis Universal texture support" ON)
OPTION(WZ_DEBUG_GFX_API_LEAKS "Enable debugging for graphics API leaks" ON)
OPTION(WZ_FORCE_MINIMAL_OPUSFILE "Force a minimal build of Opusfile, since WZ does not need (or want) HTTP stream support" ON)

if(CMAKE_SYSTEM_NAME MATCHES "Windows" OR CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "Linux")
	# Only supported on Windows, macOS, and Linux
	OPTION(ENABLE_DISCORD "Enable Discord presence / join integration" ON)
endif()

set(WZ_DISTRIBUTOR "UNKNOWN" CACHE STRING "Name of distributor compiling this package")

if (DEFINED ENV{WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES})
	# NOTE: Do *NOT* set this when building packages to distribute / install - only intended for select CI runs
	message(STATUS "WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES: \"$ENV{WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES}\"")
	set(WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES "$ENV{WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES}")
endif()
if (WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES)
	message(STATUS "WZ_CI_DISABLE_BASIS_COMPRESS_TEXTURES is set, textures will not be compressed as part of build. This should only be used for select CI runs, and *NOT* for building packages to distribute / install.")
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Disallow in-source builds
include(DisallowInSourceBuilds)

# Disable CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

if(WZ_ENABLE_WARNINGS)
	set(WZ_Wno_ "-Wno-error=")
else()
	set(WZ_Wno_ "-Wno-")
endif()

# CXX Standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions

# Support folders (for nicer IDE organization)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

# Fix: Linking issues with static libraries when using certain generated project files (ex. Xcode in "Archive" build mode)
# See: https://stackoverflow.com/questions/30318941/, https://stackoverflow.com/questions/33020245/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "./")

# Handle install paths
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# On Windows, set the default DATAROOTDIR to "."
	# - No need to put everything in a nested "share" folder, as the expectation is that
	#   WZ will be installed in its own directory (ex. a subdirectory of Program Files)
	if(NOT DEFINED CMAKE_INSTALL_DATAROOTDIR)
		set(CMAKE_INSTALL_DATAROOTDIR "." CACHE PATH "Read-only architecture-independent data root" FORCE)
	endif()
	# Set the default DOCDIR to "DATAROOTDIR/doc"
	if(NOT DEFINED CMAKE_INSTALL_DOCDIR)
		set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc" CACHE PATH "Documentation root (DATAROOTDIR/doc)" FORCE)
	endif()
endif()
include(GNUInstallDirs)
if(NOT DEFINED WZ_DATADIR)
	# Set default WZ_DATADIR based on target platform
	if(CMAKE_SYSTEM_NAME MATCHES "Windows")
		set(WZ_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/data")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(WZ_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}") # not used on macOS (macOS always creates an app bundle)
	else()
		set(WZ_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}")
	endif()
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if(NOT CMAKE_INSTALL_BINDIR STREQUAL "bin")
		# Windows builds expect a non-empty BINDIR
		# Windows NSIS installer scripts expect a BINDIR that is "bin"
		message( WARNING "Windows builds currently require CMAKE_INSTALL_BINDIR to be \"bin\" - resetting value" )
		set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "User executables (bin)" FORCE)
	endif()
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}")
	# Since the main executable install location is used to determine the install prefix,
	# an absolute bindir requires an absolute CMAKE_INSTALL_LOCALEDIR + WZ_DATADIR (or locales + data won't be found)
	set(_nonAbsolutePaths)
	if(NOT IS_ABSOLUTE "${CMAKE_INSTALL_LOCALEDIR}")
		list(APPEND _nonAbsolutePaths "CMAKE_INSTALL_LOCALEDIR")
	endif()
	if(NOT IS_ABSOLUTE "${WZ_DATADIR}")
		list(APPEND _nonAbsolutePaths "WZ_DATADIR")
	endif()
	if(_nonAbsolutePaths)
		message( FATAL_ERROR "An absolute CMAKE_INSTALL_BINDIR path cannot be used if the following are not also absolute paths: ${_nonAbsolutePaths}\nRECOMMENDED SOLUTION:\nMake all of these *relative* paths, and use CMAKE_INSTALL_DIR and/or CPACK_PACKAGING_INSTALL_PREFIX to affect the install prefix / location (which can be absolute)." )
	endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# Enable macOS-specific find scripts
	list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/macosx/cmake")

	# Workaround for framework header conflicts with find_package
	# - Common issue when Mono.framework is installed, since it contains headers for libpng (etc)
	#   and the default CMake setting finds the headers in Mono, but the library in the vcpkg
	#   install location (leading to a version mismatch)
	# - See: https://github.com/torch/image/issues/16
	set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
	# Enable detection of Homebrew-installed Gettext
	list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/gettext")
endif()

INCLUDE(AddTargetLinkFlagsIfSupported)

# Use "-fPIC" / "-fPIE" for all targets by default, including static libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_VERSION VERSION_LESS 3.14)
	# CMake < 3.14 doesn't add "-pie" by default for executables (CMake issue #14983)
	CHECK_CXX_LINKER_FLAGS("${CMAKE_EXE_LINKER_FLAGS} -pie" LINK_FLAG_PIE_SUPPORTED)
	if(LINK_FLAG_PIE_SUPPORTED AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows" AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
	endif()
elseif(POLICY CMP0083)
	cmake_policy(SET CMP0083 NEW) # Apply -pie link flag if supported
endif()

set(WZ_WIN_HAS_PDB FALSE)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	# Set better-than-default optimization flags
	add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:/Ob2>")
	add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:/GL>")
	add_link_options("$<$<CONFIG:RELWITHDEBINFO>:/INCREMENTAL:NO>")
	add_link_options("$<$<CONFIG:RELWITHDEBINFO>:/LTCG>")

	# Ensure all builds always have debug info built
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" CACHE STRING "Flags used by the linker (Release builds)" FORCE)
	set(WZ_WIN_HAS_PDB TRUE)

	# Default stack size is 1MB - increase to better match other platforms
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang" AND NOT APPLE)
	# Ensure all builds always have debug info built (Xcode is handled separately below)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
	# Ensure symbols can be demangled on Linux Debug builds
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")
endif()
if(MINGW AND ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
	# Ensure LLVM/Clang-based MINGW produces debug info in codeview format (for PDB)
	add_compile_options("-gcodeview")
	# Ensure a PDB file is created in the same location as the module
	add_link_options("-Wl,-pdb=")
	set(WZ_WIN_HAS_PDB TRUE)
endif()
if(MINGW)
	# Set default stack size to better match other platforms
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,8000000")
endif()

# Treat source files as UTF-8 (unless they have a BOM) (MSVC)
# NOTE: For MSVC, this also sets the executable character set.
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Set Windows-specific defines (ex. _WIN32_WINNT)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
		message( WARNING "WindowsStore builds are not currently supported" )
		add_definitions(-D_WIN32_WINNT=0x0A00) # Windows 10+
	else()
		add_definitions(-D_WIN32_WINNT=0x0601) # Windows 7+
	endif()
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
# Enable -fstack-clash-protection if available
check_c_compiler_flag(-fstack-clash-protection HAS_CFLAG_FSTACK_CLASH_PROTECTION)
if (HAS_CFLAG_FSTACK_CLASH_PROTECTION AND NOT (MINGW OR APPLE))
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-clash-protection")
endif()
check_cxx_compiler_flag(-fstack-clash-protection HAS_CXXFLAG_FSTACK_CLASH_PROTECTION)
if (HAS_CXXFLAG_FSTACK_CLASH_PROTECTION AND NOT (MINGW OR APPLE))
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-clash-protection")
endif()

include(CheckCompilerFlagsOutput)

set(WZ_TARGET_ADDITIONAL_PROPERTIES) # Set below to any additional properties that should be added to Warzone targets (src/, lib/*/)
set(WZ_TARGET_ADDITIONAL_C_BUILD_FLAGS) # Set below to any additional C build flags that should be added to Warzone targets (src/, lib/*/)
set(WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS) # Set below to any additional CXX build flags that should be added to Warzone targets (src/, lib/*/)

# Configure compiler warnings for WZ
# NOTE: This should be called after the 3rdparty/ libs are added, so it only affects WZ code & frameworks
macro(CONFIGURE_WZ_COMPILER_WARNINGS)
	if(MSVC)
		# MSVC

		# Enable /W4
		# NOTE: Do not use /Wall, as it enables *many* warnings that are off by default. MS recommends /W4 (max)
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/W4")

		# Enable some additional MSVC warnings (if compiling at warning level 3+)
		# C4204: nonstandard extension used : non-constant aggregate initializer
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34204")
		# C4512: 'class' : assignment operator could not be generated
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34512")

		# Enable some additional MSVC warnings (that are off by default)
		# C4005: 'identifier' : macro redefinition
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/we4005")
		# C4191: unsafe conversion from 'type of expression' to 'type required'
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34191")
		# C4263: 'function': member function does not override any base class virtual member function
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34263")
		# C4264: 'virtual_function': no override available for virtual member function from base 'class'; function is hidden
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34264")
		# C4265: 'class': class has virtual functions, but destructor is not virtual
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34265")
		# C4266: 'function' : no override available for virtual member function from base 'type'; function is hidden
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w34266")
		# C4905: wide string literal cast to 'LPSTR'
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w14905")
		# C4906: string literal cast to 'LPWSTR'
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w14906")
		# C4928: illegal copy-initialization; more than one user-defined conversion has been implicitly applied
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w14928")

		# C4289: nonstandard extension used : 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w44289")
		# C4836: nonstandard extension used : 'type' : local types or unnamed types cannot be used as template arguments
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w14836")

		# C4946: reinterpret_cast used between related classes: 'class1' and 'class2'
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/w14946")

		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable /WX
			list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/WX")
		else()
			message( STATUS "WZ will *NOT* enable /WX" )
		endif()

		# Disable some warnings for WZ (permanently)
		# C4800: 'type' : forcing value to bool 'true' or 'false' (performance warning)
		# This warning is no longer generated in Visual Studio 2017+
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4800")
		# C4127: conditional expression is constant
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4127")

		# Disable some warnings for WZ (FIXME)
		# C4100: unreferenced formal parameter
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4100")
		# C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data // FIXME!!
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4244")
		# C4702: unreachable code
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4702")
		# C4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4245")
		# C4706: assignment within conditional expression
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4706")
		# C4018: '>' : signed/unsigned mismatch
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4018")
		# C4389: '==' : signed/unsigned mismatch
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS "/wd4389")

	elseif(CMAKE_GENERATOR STREQUAL "Xcode")
		# Set Xcode generator project-level configuration + warning flags

		# Generate Xcode Schemes (on CMake 3.14+)
		if((CMAKE_MAJOR_VERSION GREATER 3) OR (CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 13))
			set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
		endif()

		# Enable Objective-C ARC
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)

		# Debugging Symbols
		set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")

		# WORKAROUND: Ensure debugging symbols are always generated for Release builds
		#
		# Required because the CMake Xcode generator (at least, as of CMake 3.11.x) automatically
		# sets "Generate Debugging Symbols" to NO for Release builds, and cannot be overridden by
		# setting CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS.
		add_compile_options($<$<CONFIG:MinSizeRel>:-g>)
		add_compile_options($<$<CONFIG:Release>:-g>)

		# Make sure the CLANG_CXX_LANGUAGE_STANDARD Xcode attribute matches the CMAKE_CXX_STANDARD
		if (CMAKE_CXX_STANDARD EQUAL 11)
			set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
		elseif (CMAKE_CXX_STANDARD EQUAL 14)
			set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
		elseif (CMAKE_CXX_STANDARD EQUAL 17)
			set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++1z")
		else()
			# Additional mapping required for CMAKE_CXX_STANDARD => Xcode's CLANG_CXX_LANGUAGE_STANDARD attribute (above)
			# Also may need to bump the minimum supported version of Xcode for compilation
			message( FATAL_ERROR "Don't know how to map from CMAKE_CXX_STANDARD \"${CMAKE_CXX_STANDARD}\" => Xcode's CLANG_CXX_LANGUAGE_STANDARD. See CMakeLists.txt" )
		endif()

		# -stdlib=libc++
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

		# Linking
		set(CMAKE_XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING YES)

		# Apple LLVM - Code Generation
		set(CMAKE_XCODE_ATTRIBUTE_GCC_NO_COMMON_BLOCKS YES)							# -fno-common

		# Apple Clang - Custom Compiler Flags
		# Custom Warning Flags (for which an Xcode attribute is not available)
		set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wall -Wextra -Wcast-align -Wwrite-strings -Wpointer-arith")

		# Custom Disabling Warning Flags (which are required because of warning flags specified above
		# and by CMake's Xcode project generator)
		set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "${CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS} -Wno-sign-compare")
		set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "${CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS} -Wno-unused-parameter")

		# Custom Warning Flags - No Error Tweaks
		set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "${CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS} -Wno-error=deprecated-declarations")

		set(_supported_cxx_compiler_flags "")

		# -Wno-shadow-field-in-constructor (Clang 3.9+)
		check_compiler_flags_output("-Werror -Wno-shadow-field-in-constructor -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-shadow-field-in-constructor" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		message( STATUS "Supported additional CXX compiler_flags=${_supported_cxx_compiler_flags}" )
		set(CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS "${CMAKE_XCODE_ATTRIBUTE_WARNING_CFLAGS} ${_supported_cxx_compiler_flags}")

		# Apple Clang - Preprocessing
		set(CMAKE_XCODE_ATTRIBUTE_ENABLE_STRICT_OBJC_MSGSEND YES)

		# Apple Clang - Warning Policies
		list(APPEND WZ_TARGET_ADDITIONAL_PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_PEDANTIC YES)
		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable GCC_TREAT_WARNINGS_AS_ERRORS for WZ targets only
			list(APPEND WZ_TARGET_ADDITIONAL_PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS YES)
		else()
			message( STATUS "WZ will *NOT* enable \"Warnings As Errors\" for WZ targets" )
		endif()

		# Apple Clang - Warnings - All languages
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING YES)		# -Wblock-capture-autoreleasing
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_CHECK_SWITCH_STATEMENTS YES)				# -Wswitch
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_DEPRECATED_FUNCTIONS YES)			# -Wdeprecated-declarations
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_DOCUMENTATION_COMMENTS NO)				# -Wdocumentation		[DISABLED]
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY YES)						# -Wempty-body
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_FOUR_CHARACTER_CONSTANTS YES)			# -Wfour-char-constants
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)								# -Wshadow
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION YES)					# -Wbool-conversion
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION YES)				# -Wconstant-conversion
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION NO)				# -Wshorten-64-to-32	[DISABLED] - FIXME
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION YES)					# -Wenum-conversion
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_FLOAT_CONVERSION NO)					# -Wfloat-conversion	[DISABLED] - FIXME
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION YES)					# -Wint-conversion
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_NON_LITERAL_NULL_CONVERSION YES)		# -Wnon-literal-null-conversion
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_IMPLICIT_SIGN_CONVERSION NO)			# -Wsign-conversion		[DISABLED] - FIXME
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INFINITE_RECURSION YES)				# -Winfinite-recursion
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED YES)		# -Wmissing-braces
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE YES)					# -Wreturn-type
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_MISSING_PARENTHESES YES)					# -Wparentheses
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS YES)	# -Wmissing-field-initializers
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_PROTOTYPES NO)				# -Wmissing-prototypes	[DISABLED]
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE YES)				# -Wnewline-eof
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ASSIGN_ENUM YES)						# -Wassign-enum (TODO: ADD BELOW TO CLANG?)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_POINTER_SIGNEDNESS YES)			# -Wpointer-sign
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SEMICOLON_BEFORE_METHOD_BODY YES)		# -Wsemicolon-before-method-body
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE NO)							# -Wsign-compare		[DISABLED]
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_STRICT_PROTOTYPES NO)					# -Wstrict-prototypes	[DISABLED]
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_COMMA YES)								# -Wcomma
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION NO)		## [DISABLED] - FIXME
		set(CMAKE_XCODE_ATTRIBUTE_GCC_TREAT_INCOMPATIBLE_POINTER_TYPE_WARNINGS_AS_ERRORS YES)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_TREAT_IMPLICIT_FUNCTION_DECLARATIONS_AS_ERRORS NO)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_TYPECHECK_CALLS_TO_PRINTF YES)			# -Wformat
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_UNGUARDED_AVAILABILITY YES_AGGRESSIVE) # -Wunguarded-availability
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS YES_AGGRESSIVE)		# -Wuninitialized
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNKNOWN_PRAGMAS YES)						# -Wunknown-pragmas
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_UNREACHABLE_CODE NO)					# -Wunreachable-code	[DISABLED] - FIXME
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION YES)						# -Wunused-function
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_LABEL YES)						# -Wunused-label
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_PARAMETER NO)						# -Wunused-parameter	[DISABLED] - FIXME?
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VALUE YES)						# -Wunused-value
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE YES)						# -Wunused-variable

		# Apple Clang - Warnings - C++
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN__EXIT_TIME_DESTRUCTORS NO)				# -Wexit-time-destructors	[DISABLED]
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_NON_VIRTUAL_DESTRUCTOR YES)				# -Wnon-virtual-dtor
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS YES)			# -Woverloaded-virtual
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_RANGE_LOOP_ANALYSIS YES)				# -Wrange-loop-analysis
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_MOVE YES)					# -Wmove
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_INVALID_OFFSETOF_MACRO YES)		# -Winvalid-offsetof
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_CXX0X_EXTENSIONS YES)					# -Wc++11-extensions

		# Apple Clang - Warnings - Objective-C
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_DIRECT_OBJC_ISA_USAGE YES_ERROR)		# -Wdeprecated-objc-isa-usage
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN__DUPLICATE_METHOD_MATCH YES)			# -Wduplicate-method-match
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_IMPLICIT_ATOMIC_PROPERTIES YES)	# -Wmplicit-atomic-properties
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_LITERAL_CONVERSION YES)			# -Wobjc-literal-conversion
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ALLOW_INCOMPLETE_PROTOCOL YES)			# -Wprotocol
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_INTERFACE_IVARS YES)				# -Wobjc-interface-ivars
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS YES)	# -Wdeprecated-implementations
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_STRICT_SELECTOR_MATCH YES)				# -Wstrict-selector-match
		set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNDECLARED_SELECTOR YES)					# -Wundeclared-selector
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_ROOT_CLASS YES_ERROR)				# -Wobjc-root-class

		# Apple LLVM - Warnings - Objective-C and ARC
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_EXPLICIT_OWNERSHIP_TYPE YES)		# -Wexplicit-ownership-type
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF YES)			# -Wimplicit-retain-self
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_OBJC_REPEATED_USE_OF_WEAK YES)			# -Warc-repeated-use-of-weak
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN__ARC_BRIDGE_CAST_NONARC YES)			# -Warc-bridge-casts-disallowed-in-nonarc

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

		# Enable -fno-common (if supported)
		check_compiler_flags_output("-Werror -fno-common -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-fno-common" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -fno-common -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-fno-common" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -fno-math-errno (if supported)
		check_compiler_flags_output("-Werror -fno-math-errno -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-fno-math-errno" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -fno-math-errno -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-fno-math-errno" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-align				(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-qual				(GCC 3.4+, Clang 3.2+ (no-op until 3.6+))
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wctor-dtor-privacy		(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0)) [C++-only]
		check_compiler_flags_output("-Werror -Wctor-dtor-privacy -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wctor-dtor-privacy" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

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

#		# NOTE: Currently disabled because of GLM
#		# -Wnoexcept				(GCC 4.6+) [C++-only]
#		check_compiler_flags_output("-Werror -Wnoexcept -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnoexcept" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

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

		# -Wshadow					(GCC 3.4+, Clang 3.2+)
		# NOTE: -Wshadow on GCC is currently too noisy, but Clang 3.9+ is much more selective
		if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.9))
			check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)

			# -Wno-shadow-field-in-constructor (Clang 3.9+)
			check_compiler_flags_output("-Werror -Wno-shadow-field-in-constructor -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-shadow-field-in-constructor" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		endif()
		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.9))
			check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

			# -Wno-shadow-field-in-constructor (Clang 3.9+)
			check_compiler_flags_output("-Werror -Wno-shadow-field-in-constructor -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-shadow-field-in-constructor" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)
		endif()

#		# FUTURE-TODO: Enable -Wuseless-cast (large number of warnings to fix)
#		# -Wuseless-cast			(GCC 4.8+)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnull-dereference		(GCC 6.0+, Clang 3.2+)
		# Temporary: Do not enable for GCC 12+ (issues with nlohmann::json + GCC 12.1 + Wnull-dereference)
		if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.0)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)
		endif()

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

		# -Wnewline-eof				(Clang 3.4+)
		check_compiler_flags_output("-Werror -Wnewline-eof -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wnewline-eof" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wnewline-eof -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnewline-eof" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wrange-loop-analysis		(Clang 3.7+)
		check_compiler_flags_output("-Werror -Wrange-loop-analysis -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wrange-loop-analysis" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcomma					(Clang 3.9+)
		check_compiler_flags_output("-Werror -Wcomma -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcomma" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcomma -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcomma" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wfloat-conversion		(GCC 4.9+, Clang 3.5+)
		check_compiler_flags_output("-Werror -Wfloat-conversion -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wfloat-conversion" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wfloat-conversion -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wfloat-conversion" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wformat-security
		check_compiler_flags_output("-Werror -Wformat-security -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wformat-security" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wformat-security -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wformat-security" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wstringop-truncation (GCC 8.0+)
		check_compiler_flags_output("-Werror -Wstringop-truncation -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wstringop-truncation" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wstringop-truncation -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wstringop-truncation" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wlogical-op (GCC 4.3+)
		check_compiler_flags_output("-Werror -Wlogical-op -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wlogical-op" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wlogical-op -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wlogical-op" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wuninitialized (GCC, Clang)
		check_compiler_flags_output("-Werror -Wuninitialized -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wuninitialized" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wuninitialized -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wuninitialized" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable -Werror (if supported)
			check_compiler_flags_output("-Werror" COMPILER_TYPE C   OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
			check_compiler_flags_output("-Werror" COMPILER_TYPE CXX OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)
		else()
			message( STATUS "WZ will *NOT* enable -Werror" )
		endif()

		# Handle -Wfloat-conversion	(warning-only for now)
		check_compiler_flags_output("-Werror ${WZ_Wno_}float-conversion -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}float-conversion" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}float-conversion -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}float-conversion" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Handle -Wunused-but-set-variable (warning-only for now)
		check_compiler_flags_output("-Werror ${WZ_Wno_}unused-but-set-variable -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}unused-but-set-variable" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}unused-but-set-variable -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}unused-but-set-variable" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Disable -Wsign-compare (FIXME)
		check_compiler_flags_output("-Werror -Wno-sign-compare -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-sign-compare" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wno-sign-compare -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-sign-compare" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Disable -Wunused-parameter (FIXME)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Disable -Wformat-truncation (FIXME?) (Test with GCC 8.0+)
		check_compiler_flags_output("-Werror -Wformat-truncation -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-format-truncation" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wformat-truncation -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-format-truncation" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		message( STATUS "Supported C compiler_flags=${_supported_c_compiler_flags}" )
		message( STATUS "Supported CXX compiler_flags=${_supported_cxx_compiler_flags}" )

		string(REPLACE " " ";" _supported_c_compiler_flags "${_supported_c_compiler_flags}")
		string(REPLACE " " ";" _supported_cxx_compiler_flags "${_supported_cxx_compiler_flags}")

		list(APPEND WZ_TARGET_ADDITIONAL_C_BUILD_FLAGS ${_supported_c_compiler_flags})
		list(APPEND WZ_TARGET_ADDITIONAL_CXX_BUILD_FLAGS ${_supported_cxx_compiler_flags})

	endif()
endmacro(CONFIGURE_WZ_COMPILER_WARNINGS)

add_definitions("-DHAVE_CONFIG_H")

# CMAKE_CURRENT_BINARY_DIR should come before the current source directory
# so that any build products are preferentially included over in-source build
# products that might have been generated by a different compilation method / run
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")
include_directories("3rdparty")
include_directories(SYSTEM "3rdparty/glm")
include_directories(SYSTEM "3rdparty/date/include")

SET(STDC_HEADERS ON)

SET(PACKAGE "warzone2100${WZ_OUTPUT_NAME_SUFFIX}")
SET(PACKAGE_BUGREPORT "http://wz2100.net/")
SET(PACKAGE_NAME "Warzone 2100")
SET(PACKAGE_TARNAME "warzone2100")
SET(PACKAGE_DISTRIBUTOR "${WZ_DISTRIBUTOR}")

SET(_ALL_SOURCE 1)
SET(_GNU_SOURCE 1)
SET(_POSIX_PTHREAD_SEMANTICS 1)
SET(_TANDEM_SOURCE 1)
if(CMAKE_SYSTEM_NAME MATCHES "SunOS") # Solaris / SunOS
	SET(__EXTENSIONS__ 1)
endif()
SET(_DARWIN_C_SOURCE 1)
SET(_MINIX OFF)
SET(_POSIX_1_SOURCE OFF)
SET(_POSIX_SOURCE OFF)
if(NOT CMAKE_SYSTEM_NAME MATCHES "BSD|DragonFly") # Do not set _XOPEN_SOURCE on FreeBSD (etc)
	SET(_XOPEN_SOURCE 700) # Enable POSIX extensions if present
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
CHECK_INCLUDE_FILES("sys/eventfd.h" HAVE_SYS_EVENTFD_H)
CHECK_INCLUDE_FILES("sys/poll.h" HAVE_SYS_POLL_H)
CHECK_INCLUDE_FILES("poll.h" HAVE_POLL_H)

INCLUDE (CheckFunctionExists)
INCLUDE (CMakePushCheckState)
INCLUDE (CheckCXXSymbolExists)
INCLUDE (CheckCXXSourceCompiles)
cmake_reset_check_state()
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_ALL_SOURCE=${_ALL_SOURCE}")
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=${_GNU_SOURCE}")
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_POSIX_PTHREAD_SEMANTICS=${_POSIX_PTHREAD_SEMANTICS}")
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_TANDEM_SOURCE=${_TANDEM_SOURCE}")
if(__EXTENSIONS__)
	list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D__EXTENSIONS__=${__EXTENSIONS__}")
endif()
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_DARWIN_C_SOURCE=${_DARWIN_C_SOURCE}")
if(_XOPEN_SOURCE)
	list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=${_XOPEN_SOURCE}")
endif()
message(STATUS "CMAKE_REQUIRED_DEFINITIONS=${CMAKE_REQUIRED_DEFINITIONS}")
CHECK_FUNCTION_EXISTS(gettext HAVE_GETTEXT)
CHECK_FUNCTION_EXISTS(iconv HAVE_ICONV)
CHECK_CXX_SYMBOL_EXISTS(strlcat "string.h" HAVE_SYSTEM_STRLCAT)
CHECK_CXX_SYMBOL_EXISTS(strlcpy "string.h" HAVE_SYSTEM_STRLCPY)
CHECK_CXX_SYMBOL_EXISTS(strlcat "string.h" HAVE_VALID_STRLCAT)
CHECK_CXX_SYMBOL_EXISTS(strlcpy "string.h" HAVE_VALID_STRLCPY)
CHECK_CXX_SYMBOL_EXISTS(putenv "stdlib.h" HAVE_PUTENV)
CHECK_CXX_SYMBOL_EXISTS(setenv "stdlib.h" HAVE_SETENV)
CHECK_CXX_SYMBOL_EXISTS(posix_spawn "spawn.h" HAVE_POSIX_SPAWN)
CHECK_CXX_SYMBOL_EXISTS(posix_spawnp "spawn.h" HAVE_POSIX_SPAWNP)
CHECK_CXX_SYMBOL_EXISTS(environ "unistd.h" HAVE_ENVIRON_DECL)
CHECK_CXX_SOURCE_COMPILES("
	#include <unistd.h>
	int main(void) {
		int fd[2];
		return pipe2(fd, 0);
	}" HAVE_PIPE2)
cmake_reset_check_state()
include(CheckCXXStdThread)
CHECK_CXX_STD_THREAD(HAVE_STD_THREAD)
cmake_reset_check_state()

set(WZ_BINDIR "${CMAKE_INSTALL_BINDIR}")
set(WZ_LOCALEDIR "${CMAKE_INSTALL_LOCALEDIR}")
message(STATUS "WZ_BINDIR=\"${WZ_BINDIR}\"")
message(STATUS "WZ_LOCALEDIR=\"${WZ_LOCALEDIR}\"")
function(CHECK_IS_ABSOLUTE_PATH _var _output)
	if(IS_ABSOLUTE "${${_var}}")
		set(${_output} ON PARENT_SCOPE)
	else()
		unset(${_output} PARENT_SCOPE)
	endif()
endfunction()
CHECK_IS_ABSOLUTE_PATH(WZ_DATADIR WZ_DATADIR_ISABSOLUTE)
CHECK_IS_ABSOLUTE_PATH(WZ_LOCALEDIR WZ_LOCALEDIR_ISABSOLUTE)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Attempt to find Miniupnpc (minimum supported API version = 9)
# NOTE: This is not available on every platform / distro
find_package(Miniupnpc 9)
if(MINIUPNPC_FOUND)
	set(WZ_USE_IMPORTED_MINIUPNPC ON)
else()
	message(STATUS "Using in-tree Miniupnpc")
	set(WZ_USE_IMPORTED_MINIUPNPC OFF)
	SET(UPNPC_BUILD_STATIC ON CACHE BOOL "miniupnpc - Build static library" FORCE)
	SET(UPNPC_BUILD_SHARED OFF CACHE BOOL "miniupnpc - Build shared library" FORCE)
	SET(UPNPC_BUILD_TESTS OFF CACHE BOOL "miniupnpc - Build tests" FORCE)
	SET(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "miniupnpc - Build samples" FORCE)
	SET(UPNPC_NO_INSTALL TRUE CACHE BOOL "miniupnpc - Disable installation" FORCE)
	add_subdirectory(3rdparty/miniupnp/miniupnpc)
	set_property(TARGET libminiupnpc-static PROPERTY FOLDER "3rdparty")
	if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND (MINGW OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT MSVC)))
		target_compile_options(miniupnpc-private INTERFACE -Wno-macro-redefined)
	endif()
endif()

add_subdirectory(3rdparty)

# Determine distribution license
set(WZ_DIST_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/dist/COPYING")
if(WZ_ENABLE_BACKEND_VULKAN OR WZ_ENABLE_BASIS_UNIVERSAL)
	# If Vulkan or Basis Universal is enabled, use the GPL3 license for distribution
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pkg/licenses/COPYING.gpl3" "${WZ_DIST_LICENSE}" COPYONLY)
else()
	# GPLv2
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/COPYING" "${WZ_DIST_LICENSE}" COPYONLY)
endif()

set(wz2100_ROOT_FILES ChangeLog AUTHORS COPYING.NONGPL "${WZ_DIST_LICENSE}" COPYING.README README.md)

CONFIGURE_WZ_COMPILER_WARNINGS()
add_subdirectory(build_tools)
add_subdirectory(data)
add_subdirectory(lib)
add_subdirectory(doc)
add_subdirectory(icons)
add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(pkg)

# Install base text / info files
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Target system is Windows
	# Must convert the wz2100_ROOT_FILES to Windows line endings, and rename with ".txt" at the end
	set(_new_wz2100_ROOT_FILES)
	foreach(rfile ${wz2100_ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		# Read in the file
		file(READ ${rfile} _contents)
		if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES "Windows") # On Windows, CMake's text-based I/O converts newlines to CRLF on file(WRITE)
			# Strip all CRs
			string(REPLACE "\r" "" _contents "${_contents}")
			# Convert all LFs to CRLFs
			string(REPLACE "\n" "\r\n" _contents "${_contents}")
		endif()
		# Write out the converted file
		set(_rfile_newfilename "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}.txt")
		file(WRITE "${_rfile_newfilename}" "${_contents}")
		list(APPEND _new_wz2100_ROOT_FILES "${_rfile_newfilename}")
	endforeach()
	set(wz2100_ROOT_FILES ${_new_wz2100_ROOT_FILES})
else()
	# Just copy the files to the build directory
	foreach(rfile ${wz2100_ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		configure_file(${rfile} "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}" COPYONLY)
	endforeach()
endif()
foreach(rfile ${wz2100_ROOT_FILES})
	install(FILES ${rfile}
			COMPONENT Info
			DESTINATION "${CMAKE_INSTALL_DOCDIR}${WZ_OUTPUT_NAME_SUFFIX}"
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
	set_property(TARGET dist PROPERTY FOLDER "_WZAliasTargets")
endif()
