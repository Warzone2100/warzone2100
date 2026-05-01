cmake_minimum_required(VERSION 3.16...3.31)

if(NOT CMAKE_SCRIPT_MODE_FILE)
	message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()

get_filename_component(_repoBase "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(DEFINED IOS_PLATFORM)
	string(TOLOWER "${IOS_PLATFORM}" _iosPlatform)
else()
	set(_iosPlatform "device")
endif()

if(_iosPlatform STREQUAL "device")
	set(_iosSysroot "iphoneos")
	if(DEFINED ENV{IOS_DEVICE_ARCHS} AND NOT "$ENV{IOS_DEVICE_ARCHS}" STREQUAL "")
		string(REPLACE "," ";" _iosArchitectures "$ENV{IOS_DEVICE_ARCHS}")
	else()
		# The bundled MoltenVK iPhoneOS static library is arm64-only. arm64e
		# hardware runs the arm64 slice, while forcing arm64e breaks linking.
		set(_iosArchitectures "arm64")
	endif()
	set(_iosTriplet "arm64-ios")
elseif(_iosPlatform STREQUAL "simulator")
	set(_iosSysroot "iphonesimulator")
	set(_iosArchitectures "arm64")
	set(_iosTriplet "arm64-ios-simulator")
else()
	message(FATAL_ERROR "Unsupported IOS_PLATFORM value: ${_iosPlatform}")
endif()

message(STATUS "iOS ${_iosPlatform} architectures: ${_iosArchitectures}")

if(DEFINED BUILD_ROOT)
	set(_buildRoot "${BUILD_ROOT}")
else()
	set(_buildRoot "${_repoBase}/platforms/ios/build/${_iosPlatform}")
endif()

file(MAKE_DIRECTORY "${_buildRoot}")
file(MAKE_DIRECTORY "${_repoBase}/platforms/ios/build/toolchain")

set(_vcpkg_root "${_repoBase}/platforms/ios/build/toolchain/vcpkg")
set(_overlay_triplets "${_repoBase}/platforms/ios/vcpkg/overlay-triplets")
set(_overlay_ports "${_repoBase}/platforms/ios/vcpkg/overlay-ports")
set(_meson_wrapper "${_repoBase}/platforms/ios/meson_apple_universal_wrapper.sh")
if(DEFINED ENV{IOS_HOST_TRIPLET} AND NOT "$ENV{IOS_HOST_TRIPLET}" STREQUAL "")
	set(_iosHostTriplet "$ENV{IOS_HOST_TRIPLET}")
else()
	execute_process(
		COMMAND /usr/bin/uname -m
		OUTPUT_VARIABLE _iosHostMachine
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(_iosHostMachine MATCHES "^(arm64|aarch64)$")
		set(_iosHostTriplet "arm64-osx")
	else()
		set(_iosHostTriplet "x64-osx")
	endif()
endif()
message(STATUS "iOS host vcpkg triplet: ${_iosHostTriplet}")
find_package(Git REQUIRED)

execute_process(
	COMMAND /bin/chmod +x "${_meson_wrapper}"
	RESULT_VARIABLE _chmod_status
)
if(NOT _chmod_status EQUAL 0)
	message(FATAL_ERROR "Failed to mark ${_meson_wrapper} executable")
endif()
set(ENV{PATH} "${_repoBase}/platforms/ios:$ENV{PATH}")

if(NOT IS_DIRECTORY "${_vcpkg_root}/.git")
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" clone -q https://github.com/Microsoft/vcpkg.git "${_vcpkg_root}"
		RESULT_VARIABLE _clone_status
	)
	if(NOT _clone_status EQUAL 0)
		message(FATAL_ERROR "Failed to clone vcpkg into ${_vcpkg_root}")
	endif()
endif()

if(NOT EXISTS "${_vcpkg_root}/vcpkg")
	execute_process(
		COMMAND ./bootstrap-vcpkg.sh
		WORKING_DIRECTORY "${_vcpkg_root}"
		RESULT_VARIABLE _bootstrap_status
	)
	if(NOT _bootstrap_status EQUAL 0)
		message(FATAL_ERROR "Failed to bootstrap vcpkg")
	endif()
endif()

execute_process(
	COMMAND ${CMAKE_COMMAND}
		-G Xcode
		-S "${_repoBase}"
		-B "${_buildRoot}"
		-DCMAKE_TOOLCHAIN_FILE=${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake
		-DVCPKG_INSTALLED_DIR=${_vcpkg_root}/installed
		-DVCPKG_TARGET_TRIPLET=${_iosTriplet}
		-DVCPKG_HOST_TRIPLET=${_iosHostTriplet}
		-DVCPKG_OVERLAY_TRIPLETS=${_overlay_triplets}
		-DVCPKG_OVERLAY_PORTS=${_overlay_ports}
		-DCMAKE_SYSTEM_NAME=iOS
		-DCMAKE_OSX_SYSROOT=${_iosSysroot}
		"-DCMAKE_OSX_ARCHITECTURES=${_iosArchitectures}"
		-DCMAKE_OSX_DEPLOYMENT_TARGET=16.0
		-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON
		-DVCPKG_MANIFEST_FEATURES=vulkan
		-DWZ_ENABLE_BACKEND_VULKAN=ON
		-DWZ_USE_SYSTEM_LIBJPEG_TURBO=OFF
		-DENABLE_NLS=OFF
		-DENABLE_DOCS=OFF
		-DENABLE_DISCORD=OFF
		-DENABLE_GNS_NETWORK_BACKEND=OFF
	RESULT_VARIABLE _configure_status
)
if(NOT _configure_status EQUAL 0)
	message(FATAL_ERROR "Failed to configure Xcode iOS project")
endif()

# Some generated targets still reference `${build}/vcpkg_installed` even when
# VCPKG_INSTALLED_DIR points at the bootstrapped vcpkg checkout.
file(MAKE_DIRECTORY "${_buildRoot}/vcpkg_installed")
set(_syncTriplets "${_iosTriplet}" "${_iosHostTriplet}")
list(REMOVE_DUPLICATES _syncTriplets)
foreach(_triplet IN LISTS _syncTriplets)
	if(IS_DIRECTORY "${_vcpkg_root}/installed/${_triplet}")
		execute_process(
			COMMAND /usr/bin/rsync -a --delete "${_vcpkg_root}/installed/${_triplet}/" "${_buildRoot}/vcpkg_installed/${_triplet}/"
			RESULT_VARIABLE _sync_status
		)
		if(NOT _sync_status EQUAL 0)
			message(FATAL_ERROR "Failed to sync ${_triplet} from ${_vcpkg_root}/installed into ${_buildRoot}/vcpkg_installed")
		endif()
	endif()
endforeach()
