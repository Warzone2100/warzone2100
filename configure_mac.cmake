cmake_minimum_required(VERSION 3.5)

# Optional input defines:
#  - VCPKG_BUILD_TYPE : This will be used to modify the current triplet (once vcpkg is downloaded)
#  - WZ_DISTRIBUTOR : Passed to the main WZ CMake configure command
#  - ADDITIONAL_VCPKG_FLAGS : Additional arguments to be passed to vcpkg
#  - ADDITIONAL_CMAKE_ARGUMENTS : Additional arguments to be passed to CMake configure
#  - ONLY_BUILD_VCPKG : Only proceed through the steps to build vcpkg
#  - SKIP_VCPKG_BUILD : Skip building vcpkg itself, proceed with remaining steps

########################################################

# To ensure reproducible builds, pin to a specific vcpkg commit
set(VCPKG_COMMIT_SHA "7f59e0013648f0dd80377330b770b414032233cb")

# WZ minimum supported macOS deployment target (< 10.9 is untested)
set(MIN_SUPPORTED_MACOSX_DEPLOYMENT_TARGET "10.9")

# Vulkan SDK
set(VULKAN_SDK_VERSION "1.3.231.1")
set(VULKAN_SDK_DL_FILENAME "vulkansdk-macos-${VULKAN_SDK_VERSION}.dmg")
set(VULKAN_SDK_DL_URL "https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/mac/${VULKAN_SDK_DL_FILENAME}?Human=true")
set(VULKAN_SDK_DL_SHA256 "ee63c647eb5108dfb663b701fd3d5e976e9826f991cbe4aaaf43b5bb01971db5")

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
# 1.) Download & extract Vulkan SDK

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
set(_HAS_VULKAN_SDK FALSE)

if((CMAKE_HOST_SYSTEM_NAME MATCHES "^Darwin$") AND (DARWIN_VERSION VERSION_GREATER_EQUAL "18.0"))

	if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "::group::Download Vulkan SDK")
	endif()
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Download Vulkan SDK...")

	set(_vulkan_sdk_out_dir "vulkansdk-macos-dl")

	execute_process(
		COMMAND ${CMAKE_COMMAND}
				-DFILENAME=${VULKAN_SDK_DL_FILENAME}
				-DURL=${VULKAN_SDK_DL_URL}
				-DEXPECTED_SHA256=${VULKAN_SDK_DL_SHA256}
				-DOUT_DIR=${_vulkan_sdk_out_dir}
				-P ${_repoBase}/macosx/configs/FetchPrebuilt.cmake
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE _exstatus
	)
	if(NOT _exstatus EQUAL 0)
		message(FATAL_ERROR "Failed to download Vulkan SDK")
	endif()
	set(_full_vulkan_dl_path "${CMAKE_CURRENT_SOURCE_DIR}/macosx/external/${_vulkan_sdk_out_dir}")

	if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::")
	endif()

	if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "::group::Extract Vulkan SDK")
	endif()
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ Extract Vulkan SDK...")

	set(_full_vulkan_install_path "${CMAKE_CURRENT_SOURCE_DIR}/macosx/external/vulkansdk-macos")
	if(EXISTS "${_full_vulkan_install_path}/.SHA256SumLoc")
		file(READ "${_full_vulkan_install_path}/.SHA256SumLoc" _strings_existing_sha256 ENCODING UTF-8)
	endif()

	if(NOT DEFINED _strings_existing_sha256 OR NOT _strings_existing_sha256 STREQUAL VULKAN_SDK_DL_SHA256)
		# Not the expected version / content, or not yet extracted
		if(EXISTS "${_full_vulkan_install_path}")
			file(REMOVE_RECURSE "${_full_vulkan_install_path}")
		endif()

		# ./InstallVulkan.app/Contents/MacOS/InstallVulkan --root ${_full_vulkan_install_path} --accept-licenses --default-answer --confirm-command install --copy_only=1
		execute_process(
			COMMAND ./InstallVulkan.app/Contents/MacOS/InstallVulkan
					--root ${_full_vulkan_install_path}
					--accept-licenses
					--default-answer
					--confirm-command install
					copy_only=1
			WORKING_DIRECTORY "${_full_vulkan_dl_path}"
			RESULT_VARIABLE _exstatus
		)
		if(NOT _exstatus EQUAL 0)
			message(FATAL_ERROR "Failed to extract Vulkan SDK (exit code: ${_exstatus})")
		endif()

		file(WRITE "${_full_vulkan_install_path}/.SHA256SumLoc" "${VULKAN_SDK_DL_SHA256}")
	endif()
	unset(_strings_existing_sha256)

	if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::")
	endif()

	# Set VULKAN_SDK environment variable, so vcpkg and CMake pick up the appropriate location
	set(ENV{VULKAN_SDK} "${_full_vulkan_install_path}/macOS")
	message(STATUS "VULKAN_SDK=$ENV{VULKAN_SDK}")
	if(NOT IS_DIRECTORY "$ENV{VULKAN_SDK}")
		message(FATAL_ERROR "Something went wrong - expected Vulkan SDK output directory does not exist: $ENV{VULKAN_SDK}")
	endif()

	set(_HAS_VULKAN_SDK TRUE)

else()

	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "-- Skipping Vulkan SDK download (compilation tools require macOS 10.14+)...")

endif()

########################################################
# 2.) Download & build vcpkg, install dependencies


if((NOT DEFINED SKIP_VCPKG_BUILD) OR NOT SKIP_VCPKG_BUILD)

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
	set(tripletFile "vcpkg/triplets/${triplet}.cmake")
	if (NOT EXISTS "${tripletFile}")
		set(tripletFile "vcpkg/triplets/community/${triplet}.cmake")
		if (NOT EXISTS "${tripletFile}")
			message(FATAL_ERROR "Unable to find VCPKG_DEFAULT_TRIPLET: ${VCPKG_DEFAULT_TRIPLET}")
		endif()
	endif()
	set(tripletCommand "set(VCPKG_BUILD_TYPE \"${VCPKG_BUILD_TYPE}\")")
	file(READ "${tripletFile}" _strings_tripletFile ENCODING UTF-8)
	string(FIND "${_strings_tripletFile}" "${tripletCommand}" _tripletCommandPos)
	if(_tripletCommandPos EQUAL -1)
		file(APPEND "${tripletFile}" "\n${tripletCommand}\n")
	else()
		message(STATUS "Already modified triplet (${triplet}) to use VCPKG_BUILD_TYPE: \"${VCPKG_BUILD_TYPE}\"")
	endif()
	unset(_tripletCommandPos)
	unset(_strings_tripletFile)
	unset(tripletCommand)
	unset(tripletFile)
	unset(triplet)
endif()

endif((NOT DEFINED SKIP_VCPKG_BUILD) OR NOT SKIP_VCPKG_BUILD)

if(DEFINED ONLY_BUILD_VCPKG AND ONLY_BUILD_VCPKG)
	message(STATUS "ONLY_BUILD_VCPKG: Stopping configure script after vcpkg build")
	return()
endif()

########################################################
## 1-e.) Download & build WZ macOS dependencies

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ vcpkg install dependencies...")

set(_additional_vcpkg_flags)
if(_HAS_VULKAN_SDK)
	set(_additional_vcpkg_flags ${ADDITIONAL_VCPKG_FLAGS} --x-no-default-features --x-feature=vulkan)
else()
	set(_additional_vcpkg_flags ${ADDITIONAL_VCPKG_FLAGS} --x-no-default-features)
endif()

set(_overlay_ports_path "${_repoBase}/.ci/vcpkg/overlay-ports")

set(_vcpkgInstallResult -1)
set(_vcpkgAttempts 0)
while(NOT _vcpkgInstallResult EQUAL 0 AND _vcpkgAttempts LESS 3)
	execute_process(
		COMMAND ./vcpkg/vcpkg install --vcpkg-root=./vcpkg/ --x-manifest-root=${_repoBase} --x-install-root=./vcpkg_installed/ --overlay-ports=${_overlay_ports_path} ${_additional_vcpkg_flags}
		RESULT_VARIABLE _vcpkgInstallResult
	)
	MATH(EXPR _vcpkgAttempts "${_vcpkgAttempts}+1")
endwhile()

if(NOT _vcpkgInstallResult EQUAL 0)
	message(FATAL_ERROR "It appears that 'vcpkg install' has failed (return code: ${_vcpkgInstallResult}) (${_vcpkgAttempts} attempts)")
endif()

unset(_vcpkgAttempts)
unset(_vcpkgInstallResult)

execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "1")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++ vcpkg install finished")

########################################################
# 3.) CMake configure (generate Xcode project)

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
