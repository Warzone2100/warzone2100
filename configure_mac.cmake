cmake_minimum_required(VERSION 3.5)

# Optional input defines:
#  - VCPKG_BUILD_TYPE : This will be used to modify the current triplet (once vcpkg is downloaded)
#  - WZ_DISTRIBUTOR : Passed to the main WZ CMake configure command

########################################################

# To ensure reproducible builds, pin to a specific vcpkg commit
set(VCPKG_COMMIT_SHA "c477e87ede1cd099c24b004624c88933644d7abd")

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

########################################################
# 1.) Download & build vcpkg, install dependencies

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

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Build vcpkg...")

execute_process(
	COMMAND ./bootstrap-vcpkg.sh
	WORKING_DIRECTORY "vcpkg"
	RESULT_VARIABLE _exstatus
)
if(NOT _exstatus EQUAL 0)
	# vcpkg currently requires modern gcc to compile itself on macOS
	message(FATAL_ERROR "vcpkg bootstrap failed - please see error output above for resolution (suggestion: brew install gcc)")
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

# Download & build WZ macOS dependencies
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

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Running CMake configure (to generate Xcode project)...")
execute_process(
	COMMAND ${CMAKE_COMMAND}
		"-DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
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
