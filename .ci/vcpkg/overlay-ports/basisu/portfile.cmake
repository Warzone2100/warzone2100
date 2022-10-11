vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF 1531cfaf9ed5232248a0a45736686a849ca3befc #tag/1.16.3
    SHA512 fdb0c4360cb8e0c85a592f6fddbf6f880c5ccbfef65c33c752fb2779b96d1edc8d4992d18321a428599a3649ace2e4843f15efe04f170085d951ddca25f87323
    HEAD_REF master
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
