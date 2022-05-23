# A simplified version of: https://github.com/nlohmann/json/blob/b21c34517900c46a943c804d9af3a20cd0c77062/CMakeLists.txt

cmake_minimum_required(VERSION 3.1)

project(nlohmann_json LANGUAGES CXX)

##
## OPTIONS
##

if (POLICY CMP0077)
    # Allow CMake 3.13+ to override options when using FetchContent / add_subdirectory.
    cmake_policy(SET CMP0077 NEW)
endif ()

option(JSON_Diagnostics         "Use extended diagnostic messages." OFF)
option(JSON_ImplicitConversions "Enable implicit conversions." OFF)
option(JSON_SystemInclude       "Include as system headers (skip for clang-tidy)." OFF)

##
## CONFIGURATION
##

set(NLOHMANN_JSON_TARGET_NAME               ${PROJECT_NAME})

set(NLOHMANN_JSON_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include/")

if (JSON_ImplicitConversions)
    message(WARNING "JSON: Implicit conversions are enabled")
endif()

if (JSON_Diagnostics)
    message(STATUS "JSON: Diagnostics enabled")
endif()

if (JSON_SystemInclude)
    set(NLOHMANN_JSON_SYSTEM_INCLUDE "SYSTEM")
endif()

##
## TARGET
## create target and add include path
##
add_library(${NLOHMANN_JSON_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${NLOHMANN_JSON_TARGET_NAME} ALIAS ${NLOHMANN_JSON_TARGET_NAME})
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    target_compile_features(${NLOHMANN_JSON_TARGET_NAME} INTERFACE cxx_range_for)
else()
    target_compile_features(${NLOHMANN_JSON_TARGET_NAME} INTERFACE cxx_std_11)
endif()

target_compile_definitions(
    ${NLOHMANN_JSON_TARGET_NAME}
    INTERFACE
    JSON_USE_IMPLICIT_CONVERSIONS=$<BOOL:${JSON_ImplicitConversions}>
    JSON_DIAGNOSTICS=$<BOOL:${JSON_Diagnostics}>
)

target_include_directories(
    ${NLOHMANN_JSON_TARGET_NAME}
    ${NLOHMANN_JSON_SYSTEM_INCLUDE} INTERFACE
    "${NLOHMANN_JSON_INCLUDE_BUILD_DIR}"
)

## add debug view definition file for msvc (natvis)
if (MSVC)
    set(NLOHMANN_ADD_NATVIS TRUE)
    set(NLOHMANN_NATVIS_FILE "nlohmann_json.natvis")
    target_sources(
        ${NLOHMANN_JSON_TARGET_NAME}
        INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/${NLOHMANN_NATVIS_FILE}"
    )
endif()
