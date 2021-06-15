if (VCPKG_TARGET_IS_LINUX)
    message(WARNING "Building with a gcc version less than 6.1 is not supported.")
    message(WARNING "${PORT} currently requires the following libraries from the system package manager:\n    libx11-dev\n    libmesa-dev\n    libxi-dev\n    libxext-dev\n\nThese can be installed on Ubuntu systems via apt-get install libx11-dev libmesa-dev libxi-dev libxext-dev.")
endif()

if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(ANGLE_CPU_BITNESS ANGLE_IS_32_BIT_CPU)
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ANGLE_CPU_BITNESS ANGLE_IS_64_BIT_CPU)
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm")
    set(ANGLE_CPU_BITNESS ANGLE_IS_32_BIT_CPU)
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(ANGLE_CPU_BITNESS ANGLE_IS_64_BIT_CPU)
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/angle
    REF 3d4f87ab5b9ba4c720cedf1f219cc0884038b140 # chromium/4472
    SHA512 30970d38770c2f9eee0b7fb9f9553e4c0b38877a47dccf7c87f912cba7822c6dd5a7728b66a5fefeee2281c250299638f24122f41f464be986105084112fec0d
    # On update check headers against opengl-registry
    PATCHES
        001-fix-uwp.patch
        002-fix-builder-error.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/angle_commit.h DESTINATION ${SOURCE_PATH})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/angle_commit.h DESTINATION ${SOURCE_PATH}/src/common)

function(checkout_in_path_with_patches PATH URL REF PATCHES)
    if(EXISTS "${PATH}")
        return()
    endif()
    
    vcpkg_from_git(
        OUT_SOURCE_PATH DEP_SOURCE_PATH
        URL "${URL}"
        REF "${REF}"
        PATCHES ${PATCHES}
    )
    file(RENAME "${DEP_SOURCE_PATH}" "${PATH}")
    file(REMOVE_RECURSE "${DEP_SOURCE_PATH}")
endfunction()

checkout_in_path_with_patches(
    "${SOURCE_PATH}/third_party/zlib"
    "https://chromium.googlesource.com/chromium/src/third_party/zlib"
    "09490503d0f201b81e03f5ca0ab8ba8ee76d4a8e"
    "third-party-zlib-far-undef.patch"
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS_DEBUG -DDISABLE_INSTALL_HEADERS=1
    OPTIONS
        -D${ANGLE_CPU_BITNESS}=1
)

vcpkg_install_cmake()

vcpkg_fixup_cmake_targets(CONFIG_PATH share/unofficial-angle TARGET_PATH share/unofficial-angle)

vcpkg_copy_pdbs()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

# File conflict with opengl-registry! Make sure headers are similar on Update!
# angle defines some additional entrypoints. 
# opengl-registry probably needs an upstream update to account for those
# Due to that all angle headers get moved to include/angle. 
# If you want to use those instead of the onces provided by opengl-registry make sure 
# VCPKG_INSTALLED_DIR/include/angle is before VCPKG_INSTALLED_DIR/include
file(GLOB_RECURSE angle_includes "${CURRENT_PACKAGES_DIR}/include")
file(COPY ${angle_includes} DESTINATION "${CURRENT_PACKAGES_DIR}/include/angle")

set(_double_files
    include/GLES/egl.h
    include/GLES/gl.h
    include/GLES/glext.h
    include/GLES/glplatform.h
    include/GLES2/gl2.h
    include/GLES2/gl2ext.h
    include/GLES2/gl2platform.h
    include/GLES3/gl3.h
    include/GLES3/gl31.h
    include/GLES3/gl32.h
    include/GLES3/gl3platform.h)
foreach(_file ${_double_files})
    if(EXISTS "${CURRENT_PACKAGES_DIR}/${_file}")
        file(REMOVE "${CURRENT_PACKAGES_DIR}/${_file}")
    endif()
endforeach()


