vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF 0b9cd40f3b42fcf44941a4d98ea767c0b10d4939 #tag/v1.15_rel2
    SHA512 b2d8cdb80d25e845a619bc34ea4aa53d83959dca9d3fbfed72dc865d76b73519cb38a63809cc5f542350fdf24c2f5d891d485e39355110f2b9006b077269be8b
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
vcpkg_copy_tools(TOOL_NAMES basisu AUTO_CLEAN)

# Remove unnecessary files
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug)

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)

vcpkg_copy_pdbs()
