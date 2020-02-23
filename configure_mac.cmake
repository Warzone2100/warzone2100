cmake_minimum_required(VERSION 3.5)

# Optional input defines:
#  - VCPKG_BUILD_TYPE : This will be used to modify the current triplet (once vcpkg is downloaded)
#  - WZ_DISTRIBUTOR : Passed to the main WZ CMake configure command
#  - ADDITIONAL_CMAKE_ARGUMENTS : Additional arguments to be passed to CMake configure

########################################################

# To ensure reproducible builds, pin to a specific vcpkg commit
set(VCPKG_COMMIT_SHA "85bf9d9d792e379e973d66c8af9f39d65d1d6d42")

# WZ macOS dependencies (for vcpkg install)
set(VCPKG_INSTALL_DEPENDENCIES physfs harfbuzz libogg libtheora libvorbis libpng sdl2 glew freetype gettext zlib)

# WZ minimum supported macOS deployment target (this is 10.10 because of Qt 5.9.x)
set(MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET "10.10")

########################################################

########################################################
# 0.) Prep-work

# Get full path to this file
if(NOT CMAKE_SCRIPT_MODE_FILE)
	message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_repoBase "${_fullPathToThisScript}" DIRECTORY) # assumes configure_mac.cmake is in the base of the repo

# Check MACOSX_DEPLOYMENT_TARGET (should be >= MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET)
if(DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
	if("$ENV{MACOSX_DEPLOYMENT_TARGET}" VERSION_LESS "${MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET}")
		message(WARNING "Environment variable MACOSX_DEPLOYMENT_TARGET ($ENV{MACOSX_DEPLOYMENT_TARGET}) is less than ${MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET}. This configuration may not be supported.")
	endif()
else()
	message(STATUS "Setting MACOSX_DEPLOYMENT_TARGET to: ${MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET}")
	set(ENV{MACOSX_DEPLOYMENT_TARGET} "${MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET}")
endif()

# Detect host OS version
if(DEFINED CMAKE_HOST_SYSTEM_VERSION)
	set(DARWIN_VERSION "${CMAKE_HOST_SYSTEM_VERSION}")
else()
	execute_process(COMMAND uname -r OUTPUT_VARIABLE DARWIN_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ CMAKE_HOST_SYSTEM_NAME (${CMAKE_HOST_SYSTEM_NAME}), DARWIN_VERSION (${DARWIN_VERSION})")

########################################################
# 1.) Download & build vcpkg, install dependencies


########################################################
## 1-a.) Download vcpkg, pin to commit

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Download vcpkg...")

find_package(Git REQUIRED)

if(NOT IS_DIRECTORY "vcpkg/.git")
	# Clone the vcpkg repo
	execute_process(
		COMMAND ${GIT_EXECUTABLE} clone -q https://github.com/Microsoft/vcpkg.git
		RESULT_VARIABLE _exstatus
	)
	if(NOT _exstatus EQUAL 0)
		message(FATAL_ERROR "Failed to clone vcpkg repo")
	endif()
else()
	# On CI (for example), the vcpkg directory may have been cached and restored
	# Fetch origin updates
	execute_process(
		COMMAND ${GIT_EXECUTABLE} fetch origin
		WORKING_DIRECTORY "vcpkg"
		RESULT_VARIABLE _exstatus
	)
	if(NOT _exstatus EQUAL 0)
		message(FATAL_ERROR "Failed to fetch vcpkg updates")
	endif()
endif()

execute_process(
	COMMAND ${GIT_EXECUTABLE} reset --hard "${VCPKG_COMMIT_SHA}"
	WORKING_DIRECTORY "vcpkg"
	RESULT_VARIABLE _exstatus
)
if(NOT _exstatus EQUAL 0)
	message(FATAL_ERROR "Failed to pin vcpkg to specific commit: ${VCPKG_COMMIT_SHA}")
endif()

########################################################
## 1-b.) Detect which version of AppleClang will be used (if --allowAppleClang is specified)

# Make a new temp directory
execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory temp_compiler_detection
    WORKING_DIRECTORY .
)

# Generate a basic CMake project file that just outputs the desired variables
file(WRITE "temp_compiler_detection/CMakeLists.txt" "\
cmake_minimum_required(VERSION 3.5)
project(detect_compiler CXX)
cmake_policy(SET CMP0025 NEW)

message(STATUS \"CMAKE_CXX_COMPILER_ID=\${CMAKE_CXX_COMPILER_ID}\")
message(STATUS \"CMAKE_CXX_COMPILER_VERSION=\${CMAKE_CXX_COMPILER_VERSION}\")
")

set(_old_env_CXX "$ENV{CXX}")
set(ENV{CXX} "clang++") # matching behavior of --allowAppleClang in vcpkg's scripts/bootstrap.sh

# Run a simple CMake configure, which will output according to the script above
execute_process(
    COMMAND ${CMAKE_COMMAND} .
    OUTPUT_VARIABLE _detection_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY temp_compiler_detection
)

set(ENV{CXX} "${_old_env_CXX}")
unset(_old_env_CXX)

# Remove the temp directory
execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove_directory temp_compiler_detection
    WORKING_DIRECTORY .
)

if(_detection_output MATCHES "CMAKE_CXX_COMPILER_ID=([^\n]*)\n")
	set(_detected_cxx_compiler_id "${CMAKE_MATCH_1}")
endif()

if(_detection_output MATCHES "CMAKE_CXX_COMPILER_VERSION=([^\n]*)\n")
	set(_detected_cxx_compiler_version "${CMAKE_MATCH_1}")
endif()

unset(_detection_output)

########################################################
## 1-c.) Determine if --allowAppleClang can be specified

set(_vcpkg_useAppleClang FALSE)
if((CMAKE_HOST_SYSTEM_NAME MATCHES "^Darwin$") AND (DARWIN_VERSION VERSION_GREATER_EQUAL "19.0"))
	# macOS 10.15+ (Darwin 19.0.0+) supports the --allowAppleClang flag on bootstrap-vcpkg.sh
	# if AppleClang >= 11.0 (Xcode 11.0+) is detected
	if((_detected_cxx_compiler_id MATCHES "^AppleClang") AND (_detected_cxx_compiler_version VERSION_GREATER_EQUAL "11.0"))
		set(_vcpkg_useAppleClang TRUE)
	endif()
endif()

set(_vcpkg_bootstrap_additional_params "")
if(_vcpkg_useAppleClang)
	set(_vcpkg_bootstrap_additional_params "--allowAppleClang")
endif()

########################################################
## 1-d.) Build vcpkg

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Build vcpkg...")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "./bootstrap-vcpkg.sh ${_vcpkg_bootstrap_additional_params}")

execute_process(
	COMMAND ${CMAKE_COMMAND} -E env --unset=MACOSX_DEPLOYMENT_TARGET ./bootstrap-vcpkg.sh ${_vcpkg_bootstrap_additional_params}
	WORKING_DIRECTORY "vcpkg"
	RESULT_VARIABLE _exstatus
)
if(NOT _exstatus EQUAL 0)
	if(_vcpkg_useAppleClang)
		message(FATAL_ERROR "vcpkg bootstrap failed - please see error output above for resolution")
	else()
		# vcpkg requires modern gcc to compile itself on macOS < 10.15 (or Xcode < 11.0)
		message(FATAL_ERROR "vcpkg bootstrap failed - please see error output above for resolution (suggestion: brew install gcc)")
	endif()
endif()

if(DEFINED VCPKG_BUILD_TYPE)
	# Add VCPKG_BUILD_TYPE to the specified triplet
	set(triplet "x64-osx") # vcpkg macOS default
	if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
		message(STATUS "Using VCPKG_DEFAULT_TRIPLET=$ENV{VCPKG_DEFAULT_TRIPLET}")
		set(triplet "$ENV{VCPKG_DEFAULT_TRIPLET}")
	endif()
	set(tripletFile "triplets/${triplet}.cmake")
	set(tripletCommand "set(VCPKG_BUILD_TYPE \"${VCPKG_BUILD_TYPE}\")")
	file(READ "vcpkg/${tripletFile}" _strings_tripletFile ENCODING UTF-8)
	string(FIND "${_strings_tripletFile}" "${tripletCommand}" _tripletCommandPos)
	if(_tripletCommandPos EQUAL -1)
		file(APPEND "vcpkg/${tripletFile}" "\n${tripletCommand}\n")
	else()
		message(STATUS "Already modified triplet (${triplet}) to use VCPKG_BUILD_TYPE: \"${VCPKG_BUILD_TYPE}\"")
	endif()
	unset(_tripletCommandPos)
	unset(_strings_tripletFile)
	unset(tripletCommand)
	unset(tripletFile)
	unset(triplet)
endif()

########################################################
## 1-e.) Download & build WZ macOS dependencies

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ vcpkg install dependencies...")
set(_vcpkgInstallResult -1)
set(_vcpkgAttempts 0)
while(NOT _vcpkgInstallResult EQUAL 0 AND _vcpkgAttempts LESS 3)
	execute_process(
		COMMAND ./vcpkg install ${VCPKG_INSTALL_DEPENDENCIES}
		WORKING_DIRECTORY "vcpkg"
		RESULT_VARIABLE _vcpkgInstallResult
	)
	MATH(EXPR _vcpkgAttempts "${_vcpkgAttempts}+1")
endwhile()

if(NOT _vcpkgInstallResult EQUAL 0)
	message(FATAL_ERROR "It appears that 'vcpkg install' has failed (return code: ${_vcpkgInstallResult}) (${_vcpkgAttempts} attempts)")
endif()

unset(_vcpkgAttempts)
unset(_vcpkgInstallResult)

# Ensure dependencies are always the desired version (in case of prior runs with different vcpkg commit / version)
execute_process(
	COMMAND ./vcpkg upgrade --no-dry-run
	WORKING_DIRECTORY "vcpkg"
	RESULT_VARIABLE _exstatus
)

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "1")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ vcpkg install finished")

########################################################
# 2.) CMake configure (generate Xcode project)

set(_additional_configure_arguments "")
if(DEFINED WZ_DISTRIBUTOR)
	set(_additional_configure_arguments "\"-DWZ_DISTRIBUTOR:STRING=${WZ_DISTRIBUTOR}\"")
endif()
if(DEFINED ADDITIONAL_CMAKE_ARGUMENTS)
	list(APPEND _additional_configure_arguments ${ADDITIONAL_CMAKE_ARGUMENTS})
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Running CMake configure (to generate Xcode project)...")
execute_process(
	COMMAND ${CMAKE_COMMAND}
		"-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
		-DGLEW_USE_STATIC_LIBS=ON
		${_additional_configure_arguments}
		-G Xcode
		"${_repoBase}"
	RESULT_VARIABLE _exstatus
)

if(NOT _exstatus EQUAL 0)
	message(FATAL_ERROR "CMake configure failed to generate Xcode project")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ CMake generated Xcode project at: ${CMAKE_CURRENT_SOURCE_DIR}/warzone2100.xcodeproj")

########################################################

unset(_exstatus)
