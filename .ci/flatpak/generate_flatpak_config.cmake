cmake_minimum_required(VERSION 3.16...3.31)

# Automatically generate a Flatpak manifest from the template file
#
# Required input defines:
# - TEMPLATE_FILE: the full filename + path for the input net.wz2100.wz2100.yaml.in template file
# - OUTPUT_FILE: the full filename + path for the output net.wz2100.wz2100.yaml file
# - PROJECT_ROOT: the path the project root (${PROJECT_SOURCE_DIR})
#
# And also, these input defines:
# - WZ_OUTPUT_NAME_SUFFIX: The desired suffix to apply to the app id in the manifest (should match the main build)
#
# If cross-compiling, specify:
# - WZ_CROSS_COMPILE_TARGET_ARCH: <aarch64, etc>
#

if(NOT DEFINED TEMPLATE_FILE OR "${TEMPLATE_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: TEMPLATE_FILE" )
endif()
if(NOT DEFINED OUTPUT_FILE OR "${OUTPUT_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: OUTPUT_FILE" )
endif()
if(NOT DEFINED PROJECT_ROOT OR "${PROJECT_ROOT}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: PROJECT_ROOT" )
endif()

if(NOT DEFINED WZ_OUTPUT_NAME_SUFFIX)
	message( FATAL_ERROR "Missing expected input define: WZ_OUTPUT_NAME_SUFFIX" )
endif()
if (DEFINED WZ_CROSS_COMPILE_TARGET_ARCH)
	message( STATUS "WZ_CROSS_COMPILE_TARGET_ARCH detected - configuring for cross-compile to: ${WZ_CROSS_COMPILE_TARGET_ARCH}" )
endif()

get_filename_component(_input_dir "${TEMPLATE_FILE}" DIRECTORY)

##################################
# Handling cross-compilation

if (DEFINED WZ_CROSS_COMPILE_TARGET_ARCH)

	if (NOT WZ_CROSS_COMPILE_TARGET_ARCH MATCHES "^(aarch64)$") # update when new target arch is needed, ensure the anchor is defined in net.wz2100.wz2100.yaml.in
		message( FATAL_ERROR "Target arch is not yet supported in the template: ${WZ_CROSS_COMPILE_TARGET_ARCH}" )
	endif()

	# Runtime target arch suffix
	set(WZ_RUNTIME_SUFFIX "/${WZ_CROSS_COMPILE_TARGET_ARCH}")

	# SDK extensions
	set(WZ_CROSS_COMPILE_SDK_EXTENSIONS "sdk-extensions:\n\
# We need to ensure the toolchain is available\n\
- org.freedesktop.Sdk.Extension.toolchain-${WZ_CROSS_COMPILE_TARGET_ARCH}\n\
# As well as the target SDK\n\
- org.freedesktop.Sdk.Compat.${WZ_CROSS_COMPILE_TARGET_ARCH}\n")

	# Cross files setup module
	set(WZ_CROSS_FILES_SETUP "- name: cross-files-setup\n\
    buildsystem: simple\n\
    cleanup:\n\
      - '*'\n\
    build-commands:\n\
      - mkdir -p /app/etc/wz-config/\n\
      - install -Dm644 Toolchain-cross-arch.cmake /app/etc/wz-config/Toolchain-cross-arch.cmake\n\
      - install -Dm644 meson-cross-file.txt /app/etc/wz-config/meson-cross-file.txt\n\
    sources:\n\
      - type: file\n\
        path: .ci/flatpak/Toolchain-cross-arch.cmake\n\
      - type: file\n\
        path: .ci/flatpak/meson-cross-file.txt\n\
")

	# Configure CMake cross-compile toolchain
	configure_file("${_input_dir}/Toolchain-cross-arch.cmake.in" ".ci/flatpak/Toolchain-cross-arch.cmake" @ONLY)
	set(WZ_CMAKE_CROSS_CONFIG_OPTIONS "- -DCMAKE_TOOLCHAIN_FILE=/app/etc/wz-config/Toolchain-cross-arch.cmake")

	# Construct simple meson cross-compile file
	configure_file("${_input_dir}/meson-cross-file.txt.in" ".ci/flatpak/meson-cross-file.txt" @ONLY)
	set(WZ_MESON_CROSS_CONFIG_OPTIONS "- --cross-file=/app/etc/wz-config/meson-cross-file.txt")

	# Autotools just requires a config option
	set(WZ_AUTOTOOLS_CROSS_CONFIG_OPTIONS "- --host=${WZ_CROSS_COMPILE_TARGET_ARCH}-unknown-linux-gnu")

	# Set the build-options anchor tag
	set(WZ_CROSS_BUILD_OPTIONS "build-options: *compat-${WZ_CROSS_COMPILE_TARGET_ARCH}-build-options")

else()
	unset(WZ_RUNTIME_SUFFIX)
	unset(WZ_CROSS_COMPILE_SDK_EXTENSIONS)
	unset(WZ_CROSS_FILES_SETUP)
	set(WZ_CMAKE_CROSS_CONFIG_OPTIONS "# No cross build options")
	set(WZ_MESON_CROSS_CONFIG_OPTIONS "# No cross build options")
	set(WZ_AUTOTOOLS_CROSS_CONFIG_OPTIONS "# No cross build options")
	set(WZ_CROSS_BUILD_OPTIONS "# No cross build options")
endif()

##################################
# Handle sentry-native

# Get the source URL AND SHA512 from a data file
set(_sentry_dl_data_file "${PROJECT_ROOT}/.sentrynative")
file(STRINGS "${_sentry_dl_data_file}" _sentry_native_url_info ENCODING UTF-8)
while(_sentry_native_url_info)
	list(POP_FRONT _sentry_native_url_info LINE)
	if (LINE MATCHES "^URL=(.*)")
		set(WZ_SENTRY_NATIVE_URL "${CMAKE_MATCH_1}")
	endif()
	if (LINE MATCHES "^SHA512=(.*)")
		set(WZ_SENTRY_NATIVE_SHA512 "${CMAKE_MATCH_1}")
	endif()
endwhile()
unset(_sentry_native_url_info)
if(NOT DEFINED WZ_SENTRY_NATIVE_URL OR NOT DEFINED WZ_SENTRY_NATIVE_SHA512)
	message(FATAL_ERROR "Failed to load URL and hash from: ${_sentry_dl_data_file}")
endif()
unset(_sentry_dl_data_file)

##################################
# Handle prebuilt-texture-packages

include("${PROJECT_ROOT}/data/WZPrebuiltPackages.cmake")

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++TEMPLATE_FILE: ${TEMPLATE_FILE}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++OUTPUT_FILE: ${OUTPUT_FILE}")

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_OUTPUT_NAME_SUFFIX: ${WZ_OUTPUT_NAME_SUFFIX}")
if (DEFINED WZ_CROSS_COMPILE_TARGET_ARCH)
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_CROSS_COMPILE_TARGET_ARCH: ${WZ_CROSS_COMPILE_TARGET_ARCH}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_CROSS_COMPILE_SDK_EXTENSIONS: ${WZ_CROSS_COMPILE_SDK_EXTENSIONS}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_CMAKE_CROSS_CONFIG_OPTIONS: ${WZ_CMAKE_CROSS_CONFIG_OPTIONS}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_MESON_CROSS_CONFIG_OPTIONS: ${WZ_MESON_CROSS_CONFIG_OPTIONS}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_AUTOTOOLS_CROSS_CONFIG_OPTIONS: ${WZ_AUTOTOOLS_CROSS_CONFIG_OPTIONS}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_CROSS_BUILD_OPTIONS: ${WZ_CROSS_BUILD_OPTIONS}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_SENTRY_NATIVE_URL: ${WZ_SENTRY_NATIVE_URL}")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++WZ_SENTRY_NATIVE_SHA512: ${WZ_SENTRY_NATIVE_SHA512}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)

