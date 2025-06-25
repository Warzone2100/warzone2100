vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF df079eca71cf83481c45059dce2165348dc1a5ea # 1.50+, including fixes
    SHA512 fc8a8c5df71b82689a6322e5d0251c469f7a7a1c1afa9c3b4217ac0ec46e7090bcc2f9a2ec5a5a36349709cc4a7718d3e679b14424d5e2ace87abbe3fa25ba81
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
    MAYBE_UNUSED_VARIABLES
        CMAKE_POLICY_VERSION_MINIMUM
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_copy_tools(TOOL_NAMES "basisu" AUTO_CLEAN)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

#vcpkg_cmake_config_fixup(PACKAGE_NAME basisu CONFIG_PATH lib/cmake/basisu)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
