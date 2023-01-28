cmake_minimum_required(VERSION 3.12)
project(crashpad LANGUAGES C CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
endif()

set(CRASHPAD_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CRASHPAD_MAIN_PROJECT ON)
endif()

option(CRASHPAD_ENABLE_INSTALL "Enable crashpad installation" "${CRASHPAD_MAIN_PROJECT}")
option(CRASHPAD_ENABLE_INSTALL_DEV "Enable crashpad development installation" "${CRASHPAD_MAIN_PROJECT}")
option(CRASHPAD_ENABLE_STACKTRACE "Enable client-side stack trace recording" OFF)

if(MSVC)
    set(CRASHPAD_ZLIB_SYSTEM_DEFAULT OFF)
else()
    set(CRASHPAD_ZLIB_SYSTEM_DEFAULT ON)
endif()
option(CRASHPAD_ZLIB_SYSTEM "Use system zlib library" "${CRASHPAD_ZLIB_SYSTEM_DEFAULT}")

if(CRASHPAD_ZLIB_SYSTEM)
    find_package(ZLIB REQUIRED)
endif()


if(LINUX OR ANDROID)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        set(CRASHPAD_USE_BORINGSSL ON)
    endif()
endif()

if (NOT (ANDROID OR FUCHSIA))
    add_compile_definitions(CRASHPAD_FLOCK_ALWAYS_SUPPORTED=1)
else()
    add_compile_definitions(CRASHPAD_FLOCK_ALWAYS_SUPPORTED=0)
endif()

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/crashpad")

function(crashpad_install_target)
    if(CRASHPAD_ENABLE_INSTALL)
        install(TARGETS ${ARGN} EXPORT crashpad_export
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif()
endfunction()
function(crashpad_install_dev)
    if(CRASHPAD_ENABLE_INSTALL_DEV)
        install(${ARGN})
    endif()
endfunction()

if(WIN32)
    enable_language(ASM_MASM)

    if(MINGW)
        find_program(JWASM_FOUND jwasm)
        if (JWASM_FOUND)
            set(CMAKE_ASM_MASM_COMPILER ${JWASM_FOUND})
            execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE COMPILER_VERSION_OUTPUT)
            if (COMPILER_VERSION_OUTPUT)
                if (COMPILER_VERSION_OUTPUT MATCHES "x86_64")
                    set(JWASM_FLAGS -win64)
                else()
                    set(JWASM_FLAGS -coff)
                endif()
            endif()
           set(CMAKE_ASM_MASM_FLAGS ${CMAKE_ASM_MASM_FLAGS} ${JWASM_FLAGS})
        endif(JWASM_FOUND)

        if(NOT CMAKE_ASM_MASM_COMPILER OR CMAKE_ASM_MASM_COMPILER STREQUAL "ml" OR CMAKE_ASM_MASM_COMPILER STREQUAL "ml64")
            message(WARNING "No custom ASM_MASM compiler defined via 'CMAKE_ASM_MASM_COMPILER'. Trying to use UASM...")
            set(CMAKE_ASM_MASM_COMPILER "uasm")
        endif()
        if(NOT CMAKE_ASM_MASM_FLAGS)
            set(CMAKE_ASM_MASM_FLAGS "-win64 -10") #use default compatibility flags
        endif()
   endif(MINGW)
else()
    enable_language(ASM)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(crashpad_interface INTERFACE)
target_include_directories(crashpad_interface INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/mini_chromium/mini_chromium>
)
target_compile_definitions(crashpad_interface INTERFACE
    CRASHPAD_LSS_SOURCE_EMBEDDED
)

if(WIN32)
    target_compile_definitions(crashpad_interface INTERFACE
        NOMINMAX
        UNICODE
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        _HAS_EXCEPTIONS=0
        _UNICODE
    )
endif()
if(MSVC)
    string(REGEX REPLACE "/[Ww][0123]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/[Ww][0123]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    target_compile_options(crashpad_interface INTERFACE
        $<$<COMPILE_LANGUAGE:C,CXX>:/FS>
        $<$<COMPILE_LANGUAGE:C,CXX>:/W4>
        $<$<COMPILE_LANGUAGE:C,CXX>:/WX>
        $<$<COMPILE_LANGUAGE:C,CXX>:/bigobj> # Support larger number of sections in obj file.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4100> # Unreferenced formal parameter.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4127> # Conditional expression is constant.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4324> # Structure was padded due to alignment specifier.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4351> # New behavior: elements of array will be default initialized.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4577> # 'noexcept' used with no exception handling mode specified.
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4996> # 'X' was declared deprecated.
    )

    # WER support is only available starting from Win10 build 10941
    if("${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" VERSION_LESS 10.0.19041)
        message(STATUS "WER support disabled. Needs target platform >= 10.0.19041 (actual: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})")
    else()
        SET(CRASHPAD_WER_ENABLED TRUE)
        SET(CRASHPAD_WER_ENABLED TRUE PARENT_SCOPE)
        message(STATUS "WER support enabled")
    endif()
elseif(MINGW)
    # redirect to wmain
    # FIXME: cmake 3.13 added target_link_options
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode")
    if(CRASHPAD_WER_ENABLED)
        message(STATUS "WER support enabled")
    else()
        message(STATUS "WER support disabled. Define CRASHPAD_WER_ENABLED = TRUE to enable.")
    endif()
endif()
add_library(crashpad::interface ALIAS crashpad_interface)

add_subdirectory(compat)
add_subdirectory(minidump)
add_subdirectory(snapshot)
add_subdirectory(util)
add_subdirectory(third_party/mini_chromium)
add_subdirectory(client)

add_subdirectory(third_party/zlib)
add_subdirectory(third_party/getopt)

add_subdirectory(tools)
add_subdirectory(handler)

if(CRASHPAD_ENABLE_STACKTRACE AND APPLE AND NOT IOS)
    set(LIBUNWIND_ENABLE_SHARED OFF)
    add_subdirectory(libunwind)
    crashpad_install_target(unwind_static)
endif()

if(CRASHPAD_ENABLE_INSTALL_DEV)
    install(EXPORT crashpad_export NAMESPACE crashpad:: FILE crashpad-targets.cmake
        DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
    include(CMakePackageConfigHelpers)
    configure_package_config_file(crashpad-config.cmake.in crashpad-config.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/crashpad-config.cmake" DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
endif()
