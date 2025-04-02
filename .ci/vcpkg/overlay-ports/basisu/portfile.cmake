vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF 42c21d8066cd4b3f33e29272b61170e3b2e243bc # post-1.60, including fixes
    SHA512 6bba489473bd5264412381491304d9e3ba35a12b3aace3ac632c6bb7e0c97b31986f07c796b0d2d19f6f61ce86b74a2dcc38113f54202081e2a3aa438abb5815
    HEAD_REF master
    PATCHES
        # Remove once https://github.com/BinomialLLC/basis_universal/pull/383 merged
        0001-cmake.patch
        # Additional WZ patches
        0002-mingw.patch
        0003-gha-msvc-workaround.patch
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
        -DEXAMPLES=OFF
        -DCMAKE_POLICY_VERSION_MINIMUM=3.31
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_copy_tools(TOOL_NAMES "basisu" AUTO_CLEAN)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

#vcpkg_cmake_config_fixup(PACKAGE_NAME basisu CONFIG_PATH lib/cmake/basisu)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
