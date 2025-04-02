cmake_minimum_required(VERSION 3.5...3.31)
project(basis_universal_host_build_wrapper)

set(BASIS_UNIVERSAL_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../basis_universal")
set(BASIS_UNIVERSAL_BUILD_SRC "${CMAKE_CURRENT_BINARY_DIR}/basis_universal_src/")

# Create copy of the source dir (the default basis universal CMake config adds files to the source dir...)
file(MAKE_DIRECTORY "${BASIS_UNIVERSAL_BUILD_SRC}")
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${BASIS_UNIVERSAL_SRC_PATH}" "${BASIS_UNIVERSAL_BUILD_SRC}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Patches:
# - Add missing install command for basisu tool
file(APPEND "${BASIS_UNIVERSAL_BUILD_SRC}/CMakeLists.txt" "\ninstall(TARGETS basisu DESTINATION bin)\n")

# Set CMAKE_POLICY_VERSION_MINIMUM if potentially needed
set(BASISU_EXTRA_CMAKE_OPTIONS "")
if(CMAKE_VERSION VERSION_GREATER 3.31)
	if(NOT DEFINED ENV{CMAKE_POLICY_VERSION_MINIMUM}) # do not override environment value, if set
		set(BASISU_EXTRA_CMAKE_OPTIONS "-DCMAKE_POLICY_VERSION_MINIMUM=3.31")
	endif()
endif()

# basis-universal
# We must build for the current (host) platform
# We DO NOT want cross-compile toolchain applied here
set(BASISU_SOURCE_DIR "${BASIS_UNIVERSAL_BUILD_SRC}")
set(BASISU_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/basis_universal_build")
set(BASISU_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/basis_install")
include(ExternalProject)
ExternalProject_Add(basisuExecutable
  SOURCE_DIR        "${BASISU_SOURCE_DIR}"
  BINARY_DIR		"${BASISU_BINARY_DIR}"
  # The two "--unset" options tell it to use the host defaults rather than the configured ones.
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env --unset=CC --unset=CXX ${CMAKE_COMMAND} chdir "${BASISU_BINARY_DIR}" ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${BASISU_INSTALL_DIR} ${BASISU_EXTRA_CMAKE_OPTIONS} "${BASISU_SOURCE_DIR}"
  BUILD_COMMAND ${CMAKE_COMMAND} --build "${BASISU_BINARY_DIR}" --target install --config Release
  INSTALL_COMMAND   ""
  #EXCLUDE_FROM_ALL  TRUE
  BUILD_ALWAYS      TRUE
  LOG_CONFIGURE TRUE
  LOG_BUILD TRUE
  LOG_INSTALL TRUE
  LOG_OUTPUT_ON_FAILURE TRUE
)
