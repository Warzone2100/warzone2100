cmake_minimum_required(VERSION 3.5)

############################
# Versioning (autorevision)

# Autorevision target that will run *every* build
add_custom_target(autorevision ALL
    DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/__shouldnotexist.h"
)

# Autorevision command that will run *every* build
add_custom_command(
    OUTPUT
		"${CMAKE_CURRENT_BINARY_DIR}/__shouldnotexist.h"  # fake - ensure we run
		"${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
		"${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache"
	# Copy autorevision.cache from the src directory (if present)
	# This supports proper versioning from distribution source archives
	COMMAND ${CMAKE_COMMAND} -DINPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/autorevision.cache" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" -DSKIP_IF_INPUT_MISSING="ON" -DSKIP_IF_OUTPUT_EXISTS="ON" -P "${CMAKE_SOURCE_DIR}/cmake/CopyFileHelper.cmake"
	# Autorevision.cmake
	# this command must generate: ${CMAKE_CURRENT_BINARY_DIR}/autorevision.h
	COMMAND ${CMAKE_COMMAND} -DCACHEFILE="${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" -DOUTPUT_TYPE=h -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/autorevision.h" -P "${CMAKE_SOURCE_DIR}/build_tools/autorevision.cmake"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# On Windows, configure warzone2100.rc and the .manifest with updated version info
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# warzone2100.manifest
	set(_manifest_template_file "${CMAKE_SOURCE_DIR}/win32/warzone2100.manifest.in")
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.manifest"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE="${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" -DPROJECT_ROOT="${PROJECT_SOURCE_DIR}" -DTEMPLATE_FILE="${_manifest_template_file}" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/warzone2100.manifest" -P "${CMAKE_SOURCE_DIR}/win32/autorevision_rc.cmake"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_manifest_template_file}" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache"
	)

	# warzone2100.rc
	set(_rc_template_file "${CMAKE_SOURCE_DIR}/win32/warzone2100.rc.in")
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE="${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" -DPROJECT_ROOT="${PROJECT_SOURCE_DIR}" -DTEMPLATE_FILE="${_rc_template_file}" -DOUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc" -P "${CMAKE_SOURCE_DIR}/win32/autorevision_rc.cmake"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_rc_template_file}" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.manifest"
	)
endif()

############################
# Main Executable

if(ENABLE_NLS)
	find_package (Intl REQUIRED)
endif()

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")
qt5_wrap_cpp(MOCFILES qtscriptdebug.h)

set(_additionalSourceFiles)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(_additionalSourceFiles "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc")
endif()

add_executable(warzone2100 ${HEADERS} ${SRC} ${MOCFILES} "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h" ${_additionalSourceFiles})
target_compile_definitions(warzone2100 PRIVATE "YY_NO_UNISTD_H")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES OUTPUT_NAME "warzone2100")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Build warzone exe as a Windows app (not a console app)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES WIN32_EXECUTABLE TRUE)

	# Add the "/MANIFEST:NO" linker flag (if supported) because a manifest is already included by warzone2100.rc
	# (This is required for MSVC builds)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/MANIFEST:NO" CACHED_RESULT_NAME LINK_FLAG_SLASH_MANFIEST_NO_SUPPORTED)
endif()

target_link_libraries(warzone2100 exception-handler gamelib ivis-opengl netplay script sdl-backend framework sequence sound widget)
target_link_libraries(warzone2100
	Qt5::Script
	Qt5::Gui
	Qt5::Widgets
	)
if(ENABLE_NLS)
	target_link_libraries(warzone2100 "${Intl_LIBRARIES}")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${SDL2MAIN_LIBRARY}" STREQUAL "")
		message( WARNING "SDL2Main library not found. Linking will not succeed." )
	endif()
    target_link_libraries(warzone2100 ws2_32 iphlpapi ${SDL2MAIN_LIBRARY})
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_libraries(warzone2100 theora)
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group,-subsystem,windows")
    target_link_libraries(warzone2100 ${QT5ALL_LIBRARIES} ${QT5ALL_LDFLAGS_OTHER})
    target_link_libraries(warzone2100 version Qt5::QWindowsIntegrationPlugin Qt5ThemeSupport Qt5FontDatabaseSupport Qt5EventDispatcherSupport)
  else()
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group")
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    target_link_libraries(warzone2100
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/theora.lib"
    )
endif()

#######################
# Additional Components

if(CMAKE_SYSTEM_NAME MATCHES "Windows")

	# Note: We cannot use CMAKE_SYSTEM_PROCESSOR to detect the target architecture because it isn't always set properly.

	include(TargetArch)
	target_architecture(CURRENT_TARGET_ARCHITECTURES)
	list(LENGTH CURRENT_TARGET_ARCHITECTURES cmake_target_arch_len)

	if(NOT "${cmake_target_arch_len}" STREQUAL "1")
		message( WARNING "Detected more than one target architecture. This is not expected behavior when targeting Windows." )
	endif()

	# Install dbghelp.dll
	if(CURRENT_TARGET_ARCHITECTURES MATCHES "x86_64")
		install(FILES
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x64/dbghelp.dll"
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x64/dbghelp.dll.license.txt"
				COMPONENT Core DESTINATION "."
		)
	elseif(CURRENT_TARGET_ARCHITECTURES MATCHES "i386")
		install(FILES
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x86/dbghelp.dll"
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x86/dbghelp.dll.license.txt"
				COMPONENT Core DESTINATION "."
		)
	else()
		# We do not currently have the dbghelp.dll for this processor architecture in the repo
		message( WARNING "Currently-unsupported target architecture for dbghelp.dll: \"${CURRENT_TARGET_ARCHITECTURES}\"; expecting x86_64 or i386" )
	endif()

endif()

#######################
# Hardening / Security

INCLUDE(AddTargetLinkFlagsIfSupported)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Enable Data Execution Prevention and Address Space Layout Randomization

	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/NXCOMPAT" CACHED_RESULT_NAME LINK_FLAG_SLASH_NXCOMPAT_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--nxcompat" CACHED_RESULT_NAME LINK_FLAG_WL_NXCOMPAT_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/DYNAMICBASE" CACHED_RESULT_NAME LINK_FLAG_SLASH_DYNAMICBASE_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--dynamicbase" CACHED_RESULT_NAME LINK_FLAG_WL_DYNAMICBASE_SUPPORTED)
endif()

# Enable RELRO (if supported)
ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,relro" CACHED_RESULT_NAME LINK_FLAG_WL_Z_RELRO_SUPPORTED)
ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,now" CACHED_RESULT_NAME LINK_FLAG_WL_Z_NOW_SUPPORTED)

#######################
# Supporting google-breakpad tools for processing minidumps

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
	# Find dump_syms.exe
	find_program(DUMP_SYMS dump_syms HINTS ${PROJECT_SOURCE_DIR})
	if(DUMP_SYMS)
		# Generate the .sym file from the .pdb
		get_target_property(_mainexename warzone2100 OUTPUT_NAME)
		add_custom_command(TARGET warzone2100 POST_BUILD
			COMMAND ${DUMP_SYMS} "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb" > "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.sym"
			DEPENDS "${DUMP_SYMS}" "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb"
		)

		# Install the .sym file (to ensure it's always with the .exe)
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.sym" COMPONENT Core DESTINATION ".")
	else()
		message( WARNING "Could not find dump_syms.exe. Unable to generate a Breakpad .sym file from .pdb" )
	endif()
endif()

#######################
# Install
install(TARGETS warzone2100 COMPONENT Core DESTINATION ".")

if(WZ_PORTABLE)
	install(FILES "${CMAKE_SOURCE_DIR}/pkg/portable.in" COMPONENT Core DESTINATION "." RENAME ".portable")
endif()

#####################
# Installing Required Runtime Dependencies

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	get_target_property(_mainexename warzone2100 OUTPUT_NAME)
	if(_mainexename)
		if(NOT CMAKE_CROSSCOMPILING)
			# Install any required runtime dependencies / DLLs (ex. from vcpkg when dynamically linking)
			install(CODE "
				if(\"\${CMAKE_BUILD_TYPE}\" STREQUAL \"Debug\")
					set(dll_source_dirs \"${CMAKE_PREFIX_PATH}/debug/bin/\")
				else()
					set(dll_source_dirs \"${CMAKE_PREFIX_PATH}/bin/\")
				endif()
				set(BU_CHMOD_BUNDLE_ITEMS ON)
				include(BundleUtilities)
				fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${_mainexename}.exe\" \"\" \"\${dll_source_dirs}\")
			" COMPONENT Core)

			# Run windeployqt to pick up all Qt dependencies
			find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
			if(TOOL_WINDEPLOYQT)
				install(CODE "
					execute_process(
						COMMAND ${TOOL_WINDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/${_mainexename}.exe\" --no-compiler-runtime --no-angle --no-svg
					)
				" COMPONENT Core)
			else()
				message( WARNING "Unable to find windeployqt; installation may not included all required Qt libraries" )
			endif()
		else()
			message( STATUS "CMAKE_CROSSCOMPILING is defined - skipping BundleUtilities" )
		endif()

		if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
			# Must install the PDB file or crash dumps won't be as useful
			install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb" COMPONENT Core DESTINATION ".")
		endif()
	else()
		message( WARNING "Unable to get OUTPUT_NAME from warzone2100 target" )
	endif()
endif()
