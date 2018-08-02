cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

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

# On macOS, generate the Info.plist file with updated version info at *build-time*
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(_info_plist_template_file "${PROJECT_SOURCE_DIR}/macosx/Resources/Warzone-Info.plist.in")
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_info_plist_template_file} -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist -P ${CMAKE_SOURCE_DIR}/macosx/cmake/autorevision_infoplist.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_info_plist_template_file}" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" "${CMAKE_SOURCE_DIR}/macosx/cmake/autorevision_infoplist.cmake"
		VERBATIM
	)

	add_custom_target(autorevision_infoplist ALL
		DEPENDS
			"${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist"
	)

	add_dependencies(autorevision_infoplist autorevision) # Ensure ordering and non-concurrency

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
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()
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
    target_link_libraries(warzone2100 ws2_32 iphlpapi)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group,-subsystem,windows")
    target_link_libraries(warzone2100 ${QT5ALL_LIBRARIES} ${QT5ALL_LDFLAGS_OTHER})
    target_link_libraries(warzone2100 version Qt5::QWindowsIntegrationPlugin Qt5ThemeSupport Qt5FontDatabaseSupport Qt5EventDispatcherSupport)
  else()
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group")
  endif()
endif()

#######################
# macOS Build Config

# Notes:
# - Resources / Data files are included in the app bundle itself
# - To better replicate a normal Xcode project, the full app bundle is created at build-time
#   (instead of waiting to bundle libraries / frameworks as part of the CMake install stage)
#   - This also enables support for Xcode's built-in code-signing workflow
# - The appropriate XCODE_LAST_KNOWN_FILE_TYPE is set on non-source files to fix "endless Xcode indexing" issues
# - A *build-time*-generated Info.plist is used, instead of CMake's default configure-time-generated Info.plist setup
#   (ensuring that the Info.plist reflects the latest autorevision info)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")

	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE TRUE)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES OUTPUT_NAME "Warzone 2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "net.wz2100.Warzone2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.wz2100.Warzone2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Warzone 2100")

	# Unset some target properties (these seem to be for Windows only?)
	set_property(TARGET warzone2100 PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE)
	set_property(TARGET warzone2100 PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG)
	set_property(TARGET warzone2100 PROPERTY LIBRARY_OUTPUT_DIRECTORY_RELEASE)
	set_property(TARGET warzone2100 PROPERTY LIBRARY_OUTPUT_DIRECTORY_DEBUG)

	# Auto-generate an Info.plist (based on autorevision info)
	add_dependencies(warzone2100 autorevision autorevision_infoplist)
	if(CMAKE_GENERATOR STREQUAL "Xcode")
		# Set the target INFOPLIST_FILE attribute to the Info.plist generated at *build-time* by autorevision_infoplist
		# (This overrides the Xcode generator's use of MACOSX_BUNDLE_INFO_PLIST, which sets a plist generated by CMake at configure-time.)
		set_target_properties(warzone2100 PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist")

		# Add a pre-build command that verifies that ENV{INFOPLIST_FILE} is set to precisely what is expected by Xcode
		# (To verify that the CMake Xcode generator does not stomp on the above attempt to manually set the INFOPLIST_FILE)
		add_custom_command(TARGET warzone2100
			PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -DNAME=INFOPLIST_FILE -DEXPECTED_VALUE=${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist -P ${CMAKE_SOURCE_DIR}/macosx/cmake/check_env.cmake
			VERBATIM
		)
	else()
		# Other generators need special-handling to properly use the Info.plist generated at *build-time* by the autorevision_infoplist target.
		message( WARNING "The generator \"${CMAKE_GENERATOR}\" is not currently fully-supported for macOS builds. See src/CMakeLists.txt" )
	endif()

	# Add the .entitlements file
	set(_wz_entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/../macosx/Resources/Warzone.entitlements")
	set_source_files_properties(
		${_wz_entitlements_file} PROPERTIES
		XCODE_LAST_KNOWN_FILE_TYPE "text.plist.entitlements"
	)
	target_sources(warzone2100 PRIVATE ${_wz_entitlements_file})
	set_target_properties(warzone2100 PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${_wz_entitlements_file}")

	# To properly bundle the resources, they must be built before the app
	add_dependencies(warzone2100 data wz2100_doc translations)

	# Bundle the Data resources
	set_property(
		SOURCE ${DATA_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data")
	set_property(
		SOURCE ${DATA_MUSIC_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data/music")
	set_source_files_properties(
		${DATA_FILES} ${DATA_MUSIC_FILES} PROPERTIES
		GENERATED TRUE
		XCODE_LAST_KNOWN_FILE_TYPE "file"
	)
	target_sources(warzone2100 PRIVATE ${DATA_FILES} ${DATA_MUSIC_FILES})

	# Add the icon
	set(_macos_app_icon "${CMAKE_CURRENT_SOURCE_DIR}/../macosx/Resources/Warzone.icns")
	set_source_files_properties(
		${_macos_app_icon} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
		XCODE_LAST_KNOWN_FILE_TYPE "image.icns"
	)
	target_sources(warzone2100 PRIVATE ${_macos_app_icon})
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_ICON_FILE "Warzone.icns")

	# Bundle the fonts
	set_property(
		SOURCE ${wz2100_fonts_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/Fonts")
	set_source_files_properties(
		${wz2100_fonts_FILES} PROPERTIES
		GENERATED TRUE
		XCODE_LAST_KNOWN_FILE_TYPE "file"
	)
	target_sources(warzone2100 PRIVATE ${wz2100_fonts_FILES})

	# Bundle the translations
	set_source_files_properties(
		"${wz2100_translations_LOCALE_FOLDER}"
		PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
		GENERATED TRUE
		XCODE_LAST_KNOWN_FILE_TYPE "folder"
	)
	target_sources(warzone2100 PRIVATE "${wz2100_translations_LOCALE_FOLDER}")
		# See xcodeproj "i18n" stage
		# Uses the wzlocal lproj files + config/LangDis to determine what to copy - should we replicate?

	# Bundle the docs
	set_property(
		SOURCE ${wz2100_doc_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/docs")
	set_property(
		SOURCE ${wz2100_doc_IMAGES_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/docs/images")
	set_source_files_properties(
		${wz2100_doc_FILES} ${wz2100_doc_IMAGES_FILES}
		PROPERTIES GENERATED TRUE)
	target_sources(warzone2100 PRIVATE ${wz2100_doc_FILES} ${wz2100_doc_IMAGES_FILES})

	# Bundle the wz2100_ROOT_FILES (COPYING, README, etc)
	if (wz2100_ROOT_FILES)
		set(_fullpath_wz2100_ROOT_FILES)
		foreach(rfile ${wz2100_ROOT_FILES})
			get_filename_component(_rfile_filename "${rfile}" NAME)
			list(APPEND _fullpath_wz2100_ROOT_FILES "${PROJECT_SOURCE_DIR}/${_rfile_filename}")
		endforeach()

		set_property(
			SOURCE ${_fullpath_wz2100_ROOT_FILES}
			PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
		target_sources(warzone2100 PRIVATE ${_fullpath_wz2100_ROOT_FILES})
	else()
		message( WARNING "wz2100_ROOT_FILES is not defined. Needed to bundle root COPYING, README, etc files." )
	endif()

	# Set Apple link flags
	set_property(TARGET warzone2100 APPEND_STRING PROPERTY LINK_FLAGS " -framework Security -framework Cocoa")

	# Embed the dSYM file in the app bundle
	set_target_properties(warzone2100 PROPERTIES
		XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "\$(CONFIGURATION_BUILD_DIR)/\$(EXECUTABLE_FOLDER_PATH)"
		XCODE_ATTRIBUTE_DWARF_DSYM_FILE_NAME "\$(EXECUTABLE_NAME).dSYM"
	)

	# Strip symbols from the final executable (Release builds)
	set_target_properties(warzone2100 PROPERTIES
		XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=MinSizeRel] YES
		XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] YES
	)

	# Frameworks
	macro(copy_imported_framework _qt_framework_target)
		get_target_property(_framework_target_imported "${_qt_framework_target}" IMPORTED)
		if(_framework_target_imported)
			get_target_property(_framework_target_type "${_qt_framework_target}" TYPE)
			if(_framework_target_type STREQUAL "INTERFACE_LIBRARY")
				get_target_property(_qt_framework_location "${_qt_framework_target}" INTERFACE_LINK_LIBRARIES)
			elseif (_framework_target_type STREQUAL "SHARED")
				get_target_property(_qt_framework_location "${_qt_framework_target}" LOCATION)
			endif()
			if(EXISTS "${_qt_framework_location}")
				# Copy framework to bundle *now* (don't wait for CMake install)
				set_source_files_properties(
					${_qt_framework_location} PROPERTIES
					MACOSX_PACKAGE_LOCATION "Frameworks"
					XCODE_LAST_KNOWN_FILE_TYPE "wrapper.framework"
					XCODE_FILE_ATTRIBUTES "CodeSignOnCopy;"
				)
				if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
					# Other generators will need a custom-command equivalent of setting the "Code Sign on Copy" Xcode attribute to ON
					message( WARNING "The generator \"${CMAKE_GENERATOR}\" does not currently handle code-signing: \"${_qt_framework_location}\"" )
				endif()
				target_sources(warzone2100 PRIVATE ${_qt_framework_location})
			else()
				message(FATAL_ERROR "Imported framework ${_qt_framework_target} not found at: ${_qt_framework_location}")
			endif()
		else()
			message(FATAL_ERROR "Framework ${_qt_framework_target} is not an IMPORTED CMake target")
		endif()
	endmacro()

	# Qt5
	copy_imported_framework(Qt5::Core)
	copy_imported_framework(Qt5::Gui)
	copy_imported_framework(Qt5::PrintSupport)	# required by QCocoaIntegrationPlugin
	copy_imported_framework(Qt5::Script)
	copy_imported_framework(Qt5::Widgets)

	# Qt5 plugins
	# based on code from CMake's QtDialog/CMakeLists.txt
	macro(copy_qt5_plugin _qt_plugin_name)
		get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
		if(EXISTS "${_qt_plugin_path}")
			get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
			get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
			get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
			set(_qt_plugin_dest "PlugIns/${_qt_plugin_type}")
			# Copy plugin to bundle *now* (don't wait for CMake install)
			set_source_files_properties(
				${_qt_plugin_path} PROPERTIES
				MACOSX_PACKAGE_LOCATION "${_qt_plugin_dest}"
				XCODE_LAST_KNOWN_FILE_TYPE "compiled.mach-o.dylib"
				XCODE_FILE_ATTRIBUTES "CodeSignOnCopy;"
			)
			if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
				# Other generators will need a custom-command equivalent of setting the "Code Sign on Copy" Xcode attribute to ON
				message( WARNING "The generator \"${CMAKE_GENERATOR}\" does not currently handle code-signing: \"${_qt_plugin_dest}\"" )
			endif()
			target_sources(warzone2100 PRIVATE ${_qt_plugin_path})
		else()
			message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found at: ${_qt_plugin_path}")
		endif()
	endmacro()

	copy_qt5_plugin("Qt5::QCocoaIntegrationPlugin")

	# Set install RPATH for WZ app bundle
	set_target_properties(warzone2100 PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")

	# Since the dependencies are bundled as part of the build stages (and not at install-time), build with the install rpath
	set_target_properties(warzone2100 PROPERTIES BUILD_WITH_INSTALL_RPATH True)

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
				COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}"
		)
	elseif(CURRENT_TARGET_ARCHITECTURES MATCHES "i386")
		install(FILES
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x86/dbghelp.dll"
				"${PROJECT_SOURCE_DIR}/win32/dbghelp/6.12/x86/dbghelp.dll.license.txt"
				COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}"
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

if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# Enable RELRO (if supported)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,relro" CACHED_RESULT_NAME LINK_FLAG_WL_Z_RELRO_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,now" CACHED_RESULT_NAME LINK_FLAG_WL_Z_NOW_SUPPORTED)
endif()

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
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.sym" COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}")
	else()
		message( WARNING "Could not find dump_syms.exe. Unable to generate a Breakpad .sym file from .pdb" )
	endif()
endif()

#######################
# Install
install(TARGETS warzone2100 COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(WZ_PORTABLE)
	install(FILES "${CMAKE_SOURCE_DIR}/pkg/portable.in" COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}" RENAME ".portable")
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
				fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${_mainexename}.exe\" \"\" \"\${dll_source_dirs}\")
			" COMPONENT Core)

			# Run windeployqt to pick up all Qt dependencies
			find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
			if(TOOL_WINDEPLOYQT)
				install(CODE "
					execute_process(
						COMMAND ${TOOL_WINDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${_mainexename}.exe\" --no-compiler-runtime --no-angle --no-svg
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
			install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb" COMPONENT Core DESTINATION "${CMAKE_INSTALL_BINDIR}")
		endif()
	else()
		message( WARNING "Unable to get OUTPUT_NAME from warzone2100 target" )
	endif()
endif()
