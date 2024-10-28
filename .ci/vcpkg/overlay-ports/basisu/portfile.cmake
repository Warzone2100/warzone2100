vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF ad9386a4a1cf2a248f7bbd45f543a7448db15267 # post-1.16.4, including fixes
    SHA512 4922af3a8d42d8c1ab551853d0ab97c0733a869cd99e95ef7a03620da023da48070a1255dcd68f6a384ee7787b5bd5dffe2cd510b2986e2a0e7181929f6ecc64
    HEAD_REF master
    PATCHES
        001-mingw.patch
        002-gha-msvc-workaround.patch
)

set(_additional_options)
if (NOT VCPKG_TARGET_ARCH MATCHES "^([xX]86|[xX]64)$")
    list(APPEND _additional_options "-DSSE=OFF")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DZSTD=ON
        ${_additional_options}
)

vcpkg_cmake_install()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
vcpkg_copy_tools(TOOL_NAMES basisu AUTO_CLEAN)

# Remove unnecessary files
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug)

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)

vcpkg_copy_pdbs()
