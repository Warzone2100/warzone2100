if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	# NSIS installer is only supported if the target platform is Windows
	return()
endif()

set(INSTALLER_VERSION "0.0.0.0" CACHE STRING "NSIS version")
set(INSTALLER_EXTDIR "" CACHE STRING "NSIS ext dir")
set(INSTALLER_COMPRESSION 1 CACHE STRING "NSIS compression level")

if(WZ_PORTABLE)
	set(OUTFILE "warzone2100_portable.exe")
	set(PACKAGE "warzone2100_portable")
	set(PACKAGE_NAME "Warzone 2100 Portable")
else()
	set(OUTFILE "warzone2100.exe")
	set(PACKAGE "warzone2100")
	set(PACKAGE_NAME "Warzone 2100")
endif()
set(PORTABLE WZ_PORTABLE)
set(PACKAGE_VERSION "master")

set(CPACK_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
SET(CPACK_GENERATOR "NSIS")
set(CPACK_COMPONENTS_ALL Core Languages)

set(CPACK_COMPONENT_CORE_DISPLAY_NAME "$(TEXT_SecBase)")
set(CPACK_COMPONENT_CORE_REQUIRED ON)
set(CPACK_COMPONENT_CORE_DESCRIPTION "$(DESC_SecBase)")

set(CPACK_COMPONENT_LANGUAGES_DISPLAY_NAME "$(TEXT_SecNLS)")
set(CPACK_COMPONENT_LANGUAGES_DESCRIPTION "$(DESC_SecNLS)")

# IMPORTANT: To generate the NSIS script, CPack uses `NSIS.template.in`

INCLUDE(CPack)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/NSIS.definitions.in
  ${CMAKE_BINARY_DIR}/NSIS.definitions.nsh
)
