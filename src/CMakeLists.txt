include(GNUInstallDirs)

############################
# Versioning (autorevision)

# IMPORTANT: Must set GENERATED property at this directory level for autorevision.h
set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)

# On Windows, configure warzone2100.rc and the .manifest with updated version info
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# warzone2100.manifest
	set(_manifest_template_file "${CMAKE_SOURCE_DIR}/win32/warzone2100.manifest.in")
	set(_manifest_output_file "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.manifest")
	add_custom_command(
		OUTPUT "${_manifest_output_file}"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_manifest_template_file} -DOUTPUT_FILE=${_manifest_output_file} -P ${CMAKE_SOURCE_DIR}/win32/autorevision_rc.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_manifest_template_file}" "${wz2100_autorevision_cache_file}"
		VERBATIM
	)

	# warzone2100.rc
	set(_rc_template_file "${CMAKE_SOURCE_DIR}/win32/warzone2100.rc.in")
	set(_rc_output_file "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc")
	add_custom_command(
		OUTPUT "${_rc_output_file}"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_rc_template_file} -DOUTPUT_FILE=${_rc_output_file} -P ${CMAKE_SOURCE_DIR}/win32/autorevision_rc.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_rc_template_file}" "${wz2100_autorevision_cache_file}" "${_manifest_output_file}"
		VERBATIM
	)

	add_custom_target(autorevision_winver ALL
		DEPENDS
			"${_manifest_output_file}"
			"${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc"
	)
	set_property(TARGET autorevision_winver PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(autorevision_winver autorevision) # Ensure ordering and non-concurrency
endif()

# On macOS, generate the Info.plist file with updated version info at *build-time*
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(_info_plist_template_file "${PROJECT_SOURCE_DIR}/macosx/Resources/Warzone-Info.plist.in")
	set(_info_plist_output_file "${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist")
	add_custom_command(
		OUTPUT "${_info_plist_output_file}"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_info_plist_template_file} -DOUTPUT_FILE=${_info_plist_output_file} -P ${CMAKE_SOURCE_DIR}/macosx/cmake/autorevision_infoplist.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_info_plist_template_file}" "${wz2100_autorevision_cache_file}" "${CMAKE_SOURCE_DIR}/macosx/cmake/autorevision_infoplist.cmake"
		VERBATIM
	)

	add_custom_target(autorevision_infoplist ALL
		DEPENDS
			"${CMAKE_CURRENT_BINARY_DIR}/Warzone-Info.plist"
	)
	set_property(TARGET autorevision_infoplist PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(autorevision_infoplist autorevision) # Ensure ordering and non-concurrency

endif()

############################
# Main Executable

if(ENABLE_NLS)
	find_package (Intl REQUIRED)
endif()

file(GLOB HEADERS "*.h" "3rdparty/*.h" "titleui/*.h" "hci/*.h" "input/*.h")
file(GLOB SRC "*.cpp" "3rdparty/*.cpp" "titleui/*.cpp" "hci/*.cpp" "input/*.cpp")

set(_additionalSourceFiles)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(_additionalSourceFiles "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.rc")
endif()

add_executable(warzone2100 ${HEADERS} ${SRC} "${wz2100_autorevision_h_file}" ${_additionalSourceFiles})
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(warzone2100)
target_compile_definitions(warzone2100 PRIVATE "YY_NO_UNISTD_H")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
set(WZ_OUTPUT_BINARY_SUFFIX "${WZ_OUTPUT_NAME_SUFFIX}")
if (WZ_SKIP_OUTPUT_NAME_SUFFIX_ON_BINARY)
	unset(WZ_OUTPUT_BINARY_SUFFIX)
endif()
SET_TARGET_PROPERTIES(warzone2100 PROPERTIES OUTPUT_NAME "warzone2100${WZ_OUTPUT_BINARY_SUFFIX}")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Build warzone exe as a Windows app (not a console app)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES WIN32_EXECUTABLE TRUE)

	# Add the "/MANIFEST:NO" linker flag (if supported) because a manifest is already included by warzone2100.rc
	# (This is required for MSVC builds)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/MANIFEST:NO" CACHED_RESULT_NAME LINK_FLAG_SLASH_MANFIEST_NO_SUPPORTED)
endif()

if(MSVC)
	# C4267: 'conversion': conversion from 'type1' to 'type2', possible loss of data // FIXME!!
	target_compile_options(warzone2100 PRIVATE "/wd4267")
endif()

target_link_libraries(warzone2100 exception-handler gamelib wzmaplib ivis-opengl netplay sdl-backend framework sequence sound widget)
target_link_libraries(warzone2100 launchinfo EmbeddedJSONSignature)
target_link_libraries(warzone2100 fmt::fmt)
if(ENABLE_NLS)
	target_link_libraries(warzone2100 ${Intl_LIBRARIES})
endif()

target_link_libraries(warzone2100 nlohmann_json)
target_link_libraries(warzone2100 optional-lite)
target_link_libraries(warzone2100 quickjs)

include(IncludeFindCurl)
target_link_libraries(warzone2100 CURL::libcurl)
target_link_libraries(warzone2100 re2::re2)

find_package(SQLite3 3.14 REQUIRED)
target_link_libraries(warzone2100 SQLite::SQLite3)
target_link_libraries(warzone2100 SQLiteCpp)

set(_curl_gnutls_thread_safe_fix FALSE)
if (DEFINED CURL_GNUTLS_REQUIRES_CALLBACKS)
	if (CURL_GNUTLS_REQUIRES_CALLBACKS STREQUAL "YES")
		# explicit gcry_control() is required when GnuTLS < 2.11.0
		find_package(LibGcrypt)
		if (LIBGCRYPT_FOUND)
			message(STATUS "LIBGCRYPT_LIBRARIES=\"${LIBGCRYPT_LIBRARIES}\"")
			message(STATUS "LIBGCRYPT_INCLUDE_DIR=\"${LIBGCRYPT_INCLUDE_DIR}\"")
			target_link_libraries(warzone2100 "${LIBGCRYPT_LIBRARIES}")
			target_include_directories(warzone2100 PRIVATE "${LIBGCRYPT_INCLUDE_DIR}")
			target_compile_definitions(warzone2100 PRIVATE "USE_OLD_GNUTLS_LOCKS_INIT")
			message(STATUS "Enabling thread-safety measures for cURL GnuTLS backend")
			set(_curl_gnutls_thread_safe_fix TRUE)
		else()
			message(WARNING "Unable to enable thread-safety callbacks for cURL GnuTLS backend; please either upgrade GnuTLS > 2.11.0 or ensure libgcrypt-dev is installed.")
		endif()
	elseif (CURL_GNUTLS_REQUIRES_CALLBACKS STREQUAL "NO")
		# no explicit lock setup is required
		message(STATUS "cURL GnuTLS backend is GnuTLS > 2.11.0; no callbacks required")
		target_compile_definitions(warzone2100 PRIVATE "CURL_GNUTLS_DOES_NOT_REQUIRE_LOCKS_INIT")
		set(_curl_gnutls_thread_safe_fix TRUE)
	else()
		message(WARNING "cURL is linked to GnuTLS, but could not find GnuTLS or determine GnuTLS version - not enabling thread-safety callbacks for GnuTLS backend")
	endif()
endif()
if (DEFINED CURL_OPENSSL_REQUIRES_CALLBACKS)
	# Check for any other thread-safe SSL backends
	set(_curl_has_thread_safe_backend FALSE)
	foreach(_backend ${CURL_SUPPORTED_SSL_BACKENDS})
		# determine if there are any non-OpenSSL, thread-safe backends enabled
		if (NOT _backend MATCHES "^OpenSSL.*")
			if (_backend STREQUAL "GnuTLS" AND _curl_gnutls_thread_safe_fix)
				set(_curl_has_thread_safe_backend TRUE)
			else()
				# All non-OpenSSL / non-GnuTLS backends supposedly do not require explicit lock configuration
				set(_curl_has_thread_safe_backend TRUE)
			endif()
		endif()
	endforeach()
	if (NOT _curl_has_thread_safe_backend)
		if (CURL_OPENSSL_REQUIRES_CALLBACKS STREQUAL "YES")
			target_link_libraries(warzone2100 OpenSSL::Crypto)
			target_compile_definitions(warzone2100 PRIVATE "USE_OPENSSL_LOCKS_INIT")
			message(STATUS "Enabling thread-safety callbacks for cURL OpenSSL backend")
		elseif (CURL_OPENSSL_REQUIRES_CALLBACKS STREQUAL "NO")
			message(STATUS "cURL OpenSSL backend (OpenSSL ${OPENSSL_VERSION}) is > 1.1.0; no callbacks required")
			target_compile_definitions(warzone2100 PRIVATE "CURL_OPENSSL_DOES_NOT_REQUIRE_LOCKS_INIT")
		else()
			message(WARNING "cURL is linked to OpenSSL, but could not find OpenSSL or determine OpenSSL version - not enabling thread-safety callbacks for OpenSSL backend")
		endif()
	else()
		message(STATUS "Ignoring cURL OpenSSL backend, as other thread-safe backend(s) exist")
	endif()
endif()
if (NOT DEFINED CURL_SUPPORTED_SSL_BACKENDS)
	if (NOT VCPKG_TOOLCHAIN) # ignore warning when using vcpkg
		message(WARNING "Could not determine cURL's SSL/TLS backends; if cURL is built with OpenSSL < 1.1.0 or GnuTLS < 2.11.0, this may result in thread-safety issues")
	endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(warzone2100 ws2_32 iphlpapi shlwapi ole32 psapi)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group,-subsystem,windows")
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--end-group")
  endif()
endif()

add_subdirectory(integrations)

if(ENABLE_DISCORD)
	if (TARGET discord-rpc AND TARGET wz-discord-integration)
		target_link_libraries(warzone2100 wz-discord-integration)
	else()
		if (NOT TARGET discord-rpc)
			message(WARNING "Missing target: discord-rpc")
		endif()
		if (NOT TARGET wz-discord-integration)
			message(WARNING "Missing target: wz-discord-integration")
		endif()
	endif()
endif()

# Dependencies
add_dependencies(warzone2100 data)
if(ENABLE_DOCS AND TARGET wz2100_doc)
	add_dependencies(warzone2100 wz2100_doc)
endif()
if(ENABLE_NLS AND TARGET translations)
	add_dependencies(warzone2100 translations)
endif()

############################
# Main App install location

# To change the install destination at configure-time, please change the value of CMAKE_INSTALL_BINDIR
# (WZ_APP_INSTALL_DEST is for the platform / generator-specific overrides *in* this file)
set(WZ_APP_INSTALL_DEST "${CMAKE_INSTALL_BINDIR}")

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

function(CopyVulkanLayerJSON _input_file _output_file _new_library_path)

	configure_file("${_input_file}" "${_output_file}" COPYONLY)

	file(READ "${_output_file}" _strings_NEW_OUTPUT_FILE
		ENCODING UTF-8
	)
	string(REGEX MATCH "(\"library_path\":[^\"]*)\"([^\"]*)\"" _library_path_match "${_strings_NEW_OUTPUT_FILE}")
	if(_library_path_match)
		set(_full_match "${CMAKE_MATCH_0}")
		set(_library_path_key "${CMAKE_MATCH_1}")
		if(CMAKE_MATCH_2)
			get_filename_component(_library_name "${CMAKE_MATCH_2}" NAME)
			string(REGEX REPLACE "${_full_match}" "${_library_path_key}\"${_new_library_path}${_library_name}\"" _strings_NEW_OUTPUT_FILE "${_strings_NEW_OUTPUT_FILE}")
		endif()
		file(WRITE "${_output_file}" "${_strings_NEW_OUTPUT_FILE}")
	endif()
	unset(_strings_NEW_OUTPUT_FILE)

endfunction()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")

	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE TRUE)
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES OUTPUT_NAME "Warzone 2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "net.wz2100.Warzone2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.wz2100.Warzone2100")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Warzone 2100")

	# Workaround: Empty "Archive" build of warzone2100 target
	# See: https://cmake.org/pipermail/cmake/2012-December/053017.html; https://gitlab.kitware.com/cmake/cmake/issues/15183
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)")
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES XCODE_ATTRIBUTE_SKIP_INSTALL "No")

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

	# Enable Hardened Runtime
	set_target_properties(warzone2100 PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")

	# To properly bundle the resources, they must be built before the app
	add_dependencies(warzone2100 data)
	if(ENABLE_DOCS)
		add_dependencies(warzone2100 wz2100_doc)
	endif()
	if(ENABLE_NLS)
		add_dependencies(warzone2100 translations)
	endif()

	# Bundle the Data resources
	set_property(
		SOURCE ${DATA_FILES}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data")
	set_property(
		SOURCE ${DATA_FONTS}
		PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data/fonts")
	foreach(_music_file ${DATA_MUSIC_FILES})
		file(RELATIVE_PATH _music_file_relative_path "${DATA_MUSIC_BASE_SOURCEDIR}" "${_music_file}")
		get_filename_component(_music_file_subdir_path "${_music_file_relative_path}" DIRECTORY)
		set_property(
			SOURCE ${_music_file}
			PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data/music/${_music_file_subdir_path}")
	endforeach()
	set_source_files_properties(
		${DATA_FILES} ${DATA_FONTS} ${DATA_MUSIC_FILES} PROPERTIES
		GENERATED TRUE
		XCODE_LAST_KNOWN_FILE_TYPE "file"
	)
	target_sources(warzone2100 PRIVATE ${DATA_FILES} ${DATA_FONTS} ${DATA_MUSIC_FILES})

	# Add the icon
	set(_macos_app_icon "${CMAKE_CURRENT_SOURCE_DIR}/../macosx/Resources/Warzone.icns")
	set_source_files_properties(
		${_macos_app_icon} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
		XCODE_LAST_KNOWN_FILE_TYPE "image.icns"
	)
	target_sources(warzone2100 PRIVATE ${_macos_app_icon})
	SET_TARGET_PROPERTIES(warzone2100 PROPERTIES MACOSX_BUNDLE_ICON_FILE "Warzone.icns")

	# Bundle the translations
	if(ENABLE_NLS)
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
	endif()

	# Bundle the docs
	if(ENABLE_DOCS)
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
	endif()

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

	if(WZ_ENABLE_BACKEND_VULKAN)
		# Vulkan / MoltenVK
		find_package(Vulkan)
		if (Vulkan_FOUND)
			message( STATUS "Vulkan_LIBRARY=${Vulkan_LIBRARY}" )
			set(_vulkan_dylibs "${Vulkan_LIBRARY}")

			# Use path to Vulkan_LIBRARY to find other dylibs in the Vulkan SDK
			get_filename_component(VulkanSDK_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)

			# Vulkan_LIBRARY realpath (Vulkan_LIBRARY is often a symlink)
			get_filename_component(_vulkan_library_realpath "${Vulkan_LIBRARY}" REALPATH)
			if (NOT _vulkan_library_realpath STREQUAL Vulkan_LIBRARY)
				list(APPEND _vulkan_dylibs "${_vulkan_library_realpath}")
			endif()

			# MoltenVK dylib
			if(EXISTS "${VulkanSDK_LIB_DIR}/libMoltenVK.dylib")
				list(APPEND _vulkan_dylibs "${VulkanSDK_LIB_DIR}/libMoltenVK.dylib")
			else()
				message( FATAL_ERROR "Missing expected libMoltenVK.dylib at: ${VulkanSDK_LIB_DIR}/libMoltenVK.dylib" )
			endif()

			# Vulkan layers
			file(GLOB _vulkan_layers LIST_DIRECTORIES false "${VulkanSDK_LIB_DIR}/libVkLayer_*.dylib")
			list(APPEND _vulkan_dylibs ${_vulkan_layers})

			# Copy _vulkan_dylibs to Frameworks folder now (don't wait for CMake install)
			foreach(_dylib ${_vulkan_dylibs})
				set_source_files_properties(
					"${_dylib}" PROPERTIES
					MACOSX_PACKAGE_LOCATION "Frameworks"
					XCODE_LAST_KNOWN_FILE_TYPE "compiled.mach-o.dylib"
				)
				if(NOT IS_SYMLINK "${_dylib}")
					set_source_files_properties(
						"${_dylib}" PROPERTIES
						XCODE_FILE_ATTRIBUTES "CodeSignOnCopy;"
					)
				endif()
				if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
					# Other generators will need a custom-command equivalent of setting the "Code Sign on Copy" Xcode attribute to ON
					message( WARNING "The generator \"${CMAKE_GENERATOR}\" does not currently handle code-signing: \"${_dylib}\"" )
				endif()
				target_sources(warzone2100 PRIVATE "${_dylib}")
			endforeach()

			set(_vulkan_resources "")
			get_filename_component(_VulkanSDK_parent "${VulkanSDK_LIB_DIR}" DIRECTORY)
			set(VulkanSDK_SHARE_VULKAN_DIR "${_VulkanSDK_parent}/share/vulkan")
			if (NOT IS_DIRECTORY "${VulkanSDK_SHARE_VULKAN_DIR}")
				message( FATAL_ERROR "Unable to find /share/vulkan at: ${VulkanSDK_SHARE_VULKAN_DIR}" )
			endif()

			# Vulkan JSON files (explicit_layer.d, icd.d)
			file(GLOB _vulkan_json LIST_DIRECTORIES false "${VulkanSDK_SHARE_VULKAN_DIR}/explicit_layer.d/*.json" "${VulkanSDK_SHARE_VULKAN_DIR}/icd.d/*.json")

			# Fix up each Vulkan JSON file
			foreach(_jsonFile ${_vulkan_json})
				get_filename_component(_jsonFile_Name "${_jsonFile}" NAME)
				string(REGEX REPLACE "^${VulkanSDK_SHARE_VULKAN_DIR}/" "" _jsonFile_SubPath "${_jsonFile}")
				get_filename_component(_jsonFile_SubPath "${_jsonFile_SubPath}" DIRECTORY)
				CopyVulkanLayerJSON("${_jsonFile}" "${CMAKE_CURRENT_BINARY_DIR}/generated_resources/vulkan/${_jsonFile_SubPath}/${_jsonFile_Name}" "../../../Frameworks/")
			endforeach()

			# Copy Vulkan configuration JSON files to Resources folder of bundle
			set_source_files_properties(
				"${CMAKE_CURRENT_BINARY_DIR}/generated_resources/vulkan"
				PROPERTIES
				MACOSX_PACKAGE_LOCATION "Resources"
				GENERATED TRUE
				XCODE_LAST_KNOWN_FILE_TYPE "folder"
			)
			target_sources(warzone2100 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated_resources/vulkan")

		else(Vulkan_FOUND)
			message(WARNING "Vulkan backend is enabled, but the Vulkan SDK cannot be found. The macOS app bundle will lack required support libraries.")
		endif(Vulkan_FOUND)
	endif(WZ_ENABLE_BACKEND_VULKAN)

	if (BUILD_SHARED_LIBS)
		# Set install RPATH for WZ app bundle
		set_target_properties(warzone2100 PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
	endif()

	# Since the dependencies are bundled as part of the build stages (and not at install-time), build with the install rpath
	set_target_properties(warzone2100 PROPERTIES BUILD_WITH_INSTALL_RPATH True)

	# Install the app bundle directly in the destination root
	set(WZ_APP_INSTALL_DEST ".")

endif()

##############################
# IDE Debug/Run Configuration

# For Visual Studio project generator
set_target_properties(warzone2100 PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:warzone2100>"
	VS_DEBUGGER_COMMAND           "$<TARGET_FILE:warzone2100>"
	VS_DEBUGGER_COMMAND_ARGUMENTS "--gfxdebug"
)

# For Xcode project generator
set_target_properties(warzone2100 PROPERTIES
	XCODE_GENERATE_SCHEME TRUE
	XCODE_SCHEME_ARGUMENTS "--gfxdebug"
	# Disable NSDocumentRevisionsDebugMode
	XCODE_SCHEME_DEBUG_DOCUMENT_VERSIONING "NO"
)

#######################
# Hardening / Security

INCLUDE(AddTargetLinkFlagsIfSupported)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Enable Data Execution Prevention and Address Space Layout Randomization

	if(MSVC)
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/NXCOMPAT" CACHED_RESULT_NAME LINK_FLAG_SLASH_NXCOMPAT_SUPPORTED)
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "/DYNAMICBASE" CACHED_RESULT_NAME LINK_FLAG_SLASH_DYNAMICBASE_SUPPORTED)
	endif()

	if(NOT MSVC)
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--nxcompat" CACHED_RESULT_NAME LINK_FLAG_WL_NXCOMPAT_SUPPORTED)
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--dynamicbase" CACHED_RESULT_NAME LINK_FLAG_WL_DYNAMICBASE_SUPPORTED)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_SIZEOF_VOID_P GREATER 8)
			ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--high-entropy-va" CACHED_RESULT_NAME LINK_FLAG_WL_HIGHENTROPYVA_SUPPORTED)
		endif()
	endif()

	if(MINGW)
		# Fix: Allow DATA imports from a DLL with a non-zero offset
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc" CACHED_RESULT_NAME LINK_FLAG_WL_RUNTIME_PSEUDO_RELOC_SUPPORTED)
		# Fix: Disable automatic image base calculation (not needed because of ASLR)
		# See: https://lists.ffmpeg.org/pipermail/ffmpeg-cvslog/2015-September/094018.html
		# See: https://sourceware.org/bugzilla/show_bug.cgi?id=19011
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--disable-auto-image-base" CACHED_RESULT_NAME LINK_FLAG_WL_DISABLE_AUTO_IMAGE_BASE_SUPPORTED)
		# Workaround a weird bug with relocation information by enabling --pic-executable
		# See: https://sourceforge.net/p/mingw-w64/mailman/message/31035280/
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,--pic-executable" CACHED_RESULT_NAME LINK_FLAG_WL_PICEXECUTABLE_SUPPORTED)
		# Fix: MinGW's LD forgets the entry point when used with pic-executable.
		# See: https://git.videolan.org/?p=ffmpeg.git;a=commitdiff;h=91b668a
		if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_SIZEOF_VOID_P GREATER 8)
			set_property(TARGET warzone2100 APPEND_STRING PROPERTY "LINK_FLAGS" " -Wl,-e,mainCRTStartup")
		else()
			set_property(TARGET warzone2100 APPEND_STRING PROPERTY "LINK_FLAGS" " -Wl,-e,_mainCRTStartup")
		endif()
		# Fix: Opt-in to extra entropy when using High-Entropy ASLR by setting the image base > 4GB
		# See: https://git.videolan.org/?p=ffmpeg.git;a=commitdiff;h=a58c22d
		if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_SIZEOF_VOID_P GREATER 8)
			# Note: The image base should be:
			#	0x140000000 - for exes
			#	0x180000000 - for DLLs
			set_property(TARGET warzone2100 APPEND_STRING PROPERTY "LINK_FLAGS" " -Wl,--image-base,0x140000000")
		endif()
	endif()
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT MSVC)
	# Ensure noexecstack
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,noexecstack" CACHED_RESULT_NAME LINK_FLAG_WL_Z_NOEXECSTACK_SUPPORTED)
	# Enable RELRO (if supported)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,relro" CACHED_RESULT_NAME LINK_FLAG_WL_Z_RELRO_SUPPORTED)
	ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-z,now" CACHED_RESULT_NAME LINK_FLAG_WL_Z_NOW_SUPPORTED)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	if (NOT BUILD_SHARED_LIBS)
		ADD_TARGET_LINK_FLAGS_IF_SUPPORTED(TARGET warzone2100 LINK_FLAGS "-Wl,-sectcreate,__RESTRICT,__restrict,/dev/null" CONFIG RELEASE CACHED_RESULT_NAME LINK_FLAG_WL_SECTCREATE_RESTRICT_SUPPORTED)
	endif()
endif()

#######################
# Supporting google-breakpad tools for processing minidumps

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_HOST_SYSTEM_NAME MATCHES "Windows" AND WZ_WIN_HAS_PDB)
	get_target_property(_mainexename warzone2100 OUTPUT_NAME)
	if(_mainexename)
		# Find dump_syms.exe
		find_program(DUMP_SYMS dump_syms HINTS "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
		if(DUMP_SYMS)
			# Generate a Breakpad .sym file
			install(CODE "
				set(DUMP_SYMS \"${DUMP_SYMS}\")
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: Running dump_syms: \${DUMP_SYMS}\")

				# Generate the .sym file from the .pdb
				set(_outputDirectory \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}\")
				set(_outputFilepath \"\${_outputDirectory}/${_mainexename}.sym\")
				execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory \"\${_outputDirectory}\")
				execute_process(COMMAND \"\${DUMP_SYMS}\" \"${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb\"
								OUTPUT_FILE \"\${_outputFilepath}\"
								RESULT_VARIABLE _dumpsyms_result)
				if(_dumpsyms_result EQUAL 0 AND EXISTS \"\${_outputFilepath}\")
					execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: Generated a Breakpad .sym file from .pdb\")
				else()
					execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: WARNING: Failed to generate a Breakpad .sym file from .pdb (exit code: \${_dumpsyms_result})\")
				endif()
			" COMPONENT DebugSymbols)
		else()
			message( WARNING "Could not find dump_syms.exe. Unable to generate a Breakpad .sym file from .pdb" )
		endif()
	else()
		message( WARNING "Unable to get OUTPUT_NAME from warzone2100 target" )
	endif()
endif()

# Support crashpad sentry backend - requires installation of helper binary to handle crashes
if(TARGET crashpad_handler)
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		message(FATAL_ERROR "crashpad_handler is not currently supported for macOS builds")
	endif()
	install(TARGETS crashpad_handler COMPONENT Core DESTINATION "${WZ_APP_INSTALL_DEST}")
	if(WIN32 AND TARGET crashpad_wer)
		# On Windows, also install the crashpad_wer.dll (if built with support)
		install(TARGETS crashpad_wer RUNTIME COMPONENT Core DESTINATION "${WZ_APP_INSTALL_DEST}")
	endif()
endif()

#######################
# Install
install(TARGETS warzone2100 COMPONENT Core DESTINATION "${WZ_APP_INSTALL_DEST}")

# For Portable packages only, copy the ".portable" file that triggers portable mode (Windows-only)
install(FILES "${CMAKE_SOURCE_DIR}/pkg/portable.in" COMPONENT PortableConfig DESTINATION "${WZ_APP_INSTALL_DEST}" RENAME ".portable" EXCLUDE_FROM_ALL)

#####################
# Installing Required Runtime Dependencies

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	get_target_property(_mainexename warzone2100 OUTPUT_NAME)
	if(_mainexename)
		if(NOT CMAKE_CROSSCOMPILING)
			# Install any required runtime dependencies / DLLs (ex. from vcpkg when dynamically linking)
			set(_wz_fixup_bundle_ignored_filenames)
			set(_wz_fixup_bundle_nocopy_libraries)
			if(MSVC)
				# Ignore system (CRT) runtimes in fixup_bundle
				# - Get a list of all of the required system libraries
				set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
				set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
				set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
				set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS TRUE)
				include(InstallRequiredSystemLibraries)
				# - CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS should now contain the runtime files (full paths)
				# - Extract just the filenames
				foreach(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
					get_filename_component(lib_name "${lib}" NAME)
					list(APPEND _wz_fixup_bundle_ignored_filenames "${lib_name}")
					list(APPEND _wz_fixup_bundle_nocopy_libraries "${lib_name}")
				endforeach()
				if(NOT _wz_fixup_bundle_ignored_filenames)
					message( WARNING "InstallRequiredSystemLibraries returned no libraries? (CMake: ${CMAKE_VERSION}; MSVC: ${MSVC_VERSION})" )
				endif()
			endif()
			# - Always ignore dbghelp.dll
			list(APPEND _wz_fixup_bundle_ignored_filenames "dbghelp.dll")
			list(APPEND _wz_fixup_bundle_nocopy_libraries "dbghelp.dll")
			if(MINGW)
				set(_additional_lookup_options)
				if(DEFINED WZ_MINGW_DIST_PATH)
					message(STATUS "WZ_MINGW_DIST_PATH: \"${WZ_MINGW_DIST_PATH}\"")
					set(_additional_lookup_options HINTS "${WZ_MINGW_DIST_PATH}")
				endif()
				# Find path to C++ stdlib, and preserve for later fixup_bundle use
				find_file(WZ_STDCXXDLL NAMES "libc++.dll" "libstdc++-6.dll" ${_additional_lookup_options})
				if(WZ_STDCXXDLL)
					message(STATUS "Detected C++ stdlib DLL: ${WZ_STDCXXDLL}")
					get_filename_component(WZ_STDCXXDLL_PATH "${WZ_STDCXXDLL}" DIRECTORY)
				else()
					message(WARNING "Did not find a path to the C++ stdlib DLL. Packaged binary may not run on other systems.")
				endif()
			endif()
			if(_wz_fixup_bundle_ignored_filenames)
				message( STATUS "fixup_bundle: IGNORE_ITEM ${_wz_fixup_bundle_ignored_filenames}" )
			endif()
			set(_wz_cmake_module_path "${PROJECT_SOURCE_DIR}/cmake")
			install(CODE "
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: CMAKE_INSTALL_CONFIG_NAME: \${CMAKE_INSTALL_CONFIG_NAME}\")
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}\")
				# vcpkg's vcpkg.cmake adds both the release and debug prefix paths to the CMAKE_PREFIX_PATH:
				# - ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug # (debug prefix)
				# - ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET} # (release prefix)
				# the debug path is a subdirectory of the main / release path
				# remove the debug path, leaving just the base prefix
				set(wz_vcpkg_installed_prefix_path \"${CMAKE_PREFIX_PATH}\")
				list(FILTER wz_vcpkg_installed_prefix_path EXCLUDE REGEX \"/debug$\")
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: vcpkg_installed_prefix_path: \${wz_vcpkg_installed_prefix_path}\")
				if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" MATCHES \"^([Dd][Ee][Bb][Uu][Gg])$\")
					set(dll_source_dir_vcpkg \"\${wz_vcpkg_installed_prefix_path}/debug/bin/\")
				else()
					set(dll_source_dir_vcpkg \"\${wz_vcpkg_installed_prefix_path}/bin/\")
				endif()
				set(dll_source_dirs \"\${dll_source_dir_vcpkg}\")
				# MINGW may require distributing the C++ stdlib DLLs
				set(_WZ_STDCXXDLL_PATH \"${WZ_STDCXXDLL_PATH}\")
				if(_WZ_STDCXXDLL_PATH AND EXISTS \"\${_WZ_STDCXXDLL_PATH}\")
					execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: C++ stdlib path: \${_WZ_STDCXXDLL_PATH}\")
					# Add second, to ensure that vcpkg-built dependencies are always selected first
					list(APPEND dll_source_dirs \"\${_WZ_STDCXXDLL_PATH}\")
				endif()
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: dll_source_dirs: \${dll_source_dirs}\")
				set(_ignored_filenames \"${_wz_fixup_bundle_ignored_filenames}\")
				if(_ignored_filenames)
					set(_wz_fixup_bundle_ignore_item \"IGNORE_ITEM \\\"\${_ignored_filenames}\\\"\")
				else()
					set(_wz_fixup_bundle_ignore_item)
				endif()
				set(BU_CHMOD_BUNDLE_ITEMS ON)
				include(BundleUtilities)
				fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/${_mainexename}.exe\" \"\" \"\${dll_source_dirs}\" ${_wz_fixup_bundle_ignore_item})
				# Passing IGNORE_ITEM to fixup_bundle does not prevent fixup_bundle from copying the ignored items themselves to the BINDIR
				# Iterate over _wz_fixup_bundle_nocopy_libraries and remove them if they've been copied
				set(_nocopy_libs \"${_wz_fixup_bundle_nocopy_libraries}\")
				foreach(lib \${_nocopy_libs})
					set(_lib_fullpath \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\${lib}\")
					if(EXISTS \"\${_lib_fullpath}\")
						execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++Removing lib: \${lib}\")
						file(REMOVE \"\${_lib_fullpath}\")
					endif()
				endforeach()
				# Sanity-check to ensure that the C++ stdlib, for mingw, actually came from mingw (i.e. is the expected version + architecture)
				set(_WZ_STDCXXDLL \"${WZ_STDCXXDLL}\")
				if(_WZ_STDCXXDLL AND EXISTS \"\${_WZ_STDCXXDLL}\")
					file(SHA512 \"\${_WZ_STDCXXDLL}\" _std_cxx_expected_hash)
					get_filename_component(_WZ_STDCXXDLL_FILENAME \"\${_WZ_STDCXXDLL}\" NAME)
					set(_copied_cxx_stdlib \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\${_WZ_STDCXXDLL_FILENAME}\")
					file(SHA512 \"\${_copied_cxx_stdlib}\" _std_cxx_actual_hash)
					if(\"\${_std_cxx_expected_hash}\" STREQUAL \"\${_std_cxx_actual_hash}\")
						execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++Verified expected C++ stdlib: \${_WZ_STDCXXDLL_FILENAME}\")
					else()
						message(FATAL_ERROR \"Copied C++ stdlib does not match expected C++ stdlib\")
					endif()
					include(\"${_wz_cmake_module_path}/GetPEFileArch.cmake\")
					get_pe_file_machine_raw_bytes(\"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/${_mainexename}.exe\" _wz_exe_machine_bytes_hex)
					get_pe_file_machine_raw_bytes(\"\${_copied_cxx_stdlib}\" _cxx_stdlib_machine_bytes_hex)
					pe_file_machine_raw_bytes_to_arch(\"\${_wz_exe_machine_bytes_hex}\" _wz_exe_arch)
					if(_wz_exe_machine_bytes_hex STREQUAL _cxx_stdlib_machine_bytes_hex)
						execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++Verified expected C++ stdlib architecture matches WZ (\${_wz_exe_arch}): \${_WZ_STDCXXDLL_FILENAME}\")
					else()
						message(FATAL_ERROR \"Copied C++ stdlib architecture mismatch (WZ: \${_wz_exe_machine_bytes_hex}; \${_copied_cxx_stdlib}: \${_cxx_stdlib_machine_bytes_hex}\")
					endif()
					if(NOT CMAKE_VERSION VERSION_LESS 3.16) # file(GET_RUNTIME_DEPENDENCIES ...) requires CMake 3.16+
						file(GET_RUNTIME_DEPENDENCIES
							RESOLVED_DEPENDENCIES_VAR _WZ_STDCXXDLL_DEPENDENCIES
							UNRESOLVED_DEPENDENCIES_VAR _WZ_STDCXXDLL_UNRESOLVED_DEPENDENCIES
							LIBRARIES \"\${_WZ_STDCXXDLL}\"
							DIRECTORIES \"\${_WZ_STDCXXDLL_PATH}\"
							PRE_EXCLUDE_REGEXES \"^api-ms-win-*.dll$\" \"^ucrtbase.dll$\" \"^kernelbase.dll$\" \"^kernel32.dll$\"
							POST_EXCLUDE_REGEXES \"[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/][Ss][Yy][Ss][Tt][Ee][Mm]32[\\\\/].*\.dll$\"
						)
						foreach(_stdcxx_dependency IN LISTS _WZ_STDCXXDLL_DEPENDENCIES)
							get_filename_component(_stdcxx_dependency_filename \"\${_stdcxx_dependency}\" NAME)
							if(EXISTS \"\${_WZ_STDCXXDLL_PATH}/\${_stdcxx_dependency_filename}\")
								file(SHA512 \"\${_WZ_STDCXXDLL_PATH}/\${_stdcxx_dependency_filename}\" _stdcxx_dependency_expected_hash)
								if(EXISTS \"\${_WZ_STDCXXDLL_PATH}/\${_stdcxx_dependency_filename}\")
									file(SHA512 \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\${_stdcxx_dependency_filename}\" _stdcxx_dependency_actual_hash)
									if(\"\${_stdcxx_dependency_expected_hash}\" STREQUAL \"\${_stdcxx_dependency_actual_hash}\")
										execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++Verified expected C++ stdlib dependency: \${_stdcxx_dependency_filename}\")
									else()
										message(FATAL_ERROR \"Copied C++ stdlib dependency does not match expected: \${_stdcxx_dependency_filename}\")
									endif()
									get_pe_file_machine_raw_bytes(\"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\${_stdcxx_dependency_filename}\" _stdcxx_dependency_machine_bytes_hex)
									if(NOT _wz_exe_machine_bytes_hex STREQUAL _stdcxx_dependency_machine_bytes_hex)
										message(FATAL_ERROR \"Copied C++ library architecture mismatch (WZ: \${_wz_exe_machine_bytes_hex}; \${_stdcxx_dependency_filename}: \${_stdcxx_dependency_machine_bytes_hex}\")
									endif()
								endif()
							endif()
						endforeach()
					endif()
				endif()
			" COMPONENT Core)

			# Install additional PDBs (from vcpkg-built dependencies)
			install(CODE "
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: CMAKE_INSTALL_CONFIG_NAME: \${CMAKE_INSTALL_CONFIG_NAME}\")
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}\")
				# vcpkg's vcpkg.cmake adds both the release and debug prefix paths to the CMAKE_PREFIX_PATH:
				# - ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug # (debug prefix)
				# - ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET} # (release prefix)
				# the debug path is a subdirectory of the main / release path
				# remove the debug path, leaving just the base prefix
				set(wz_vcpkg_installed_prefix_path \"${CMAKE_PREFIX_PATH}\")
				list(FILTER wz_vcpkg_installed_prefix_path EXCLUDE REGEX \"/debug$\")
				list(GET wz_vcpkg_installed_prefix_path 0 wz_vcpkg_installed_prefix_path)
				execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: vcpkg_installed_prefix_path: \${wz_vcpkg_installed_prefix_path}\")
				if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" MATCHES \"^([Dd][Ee][Bb][Uu][Gg])$\")
					set(dll_source_dir_vcpkg \"\${wz_vcpkg_installed_prefix_path}/debug/bin/\")
				else()
					set(dll_source_dir_vcpkg \"\${wz_vcpkg_installed_prefix_path}/bin/\")
				endif()
				file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\")
				# Try to copy .PDB files for each of the DLLs (from vcpkg) as well
				file(GLOB _vcpkg_pdbs LIST_DIRECTORIES false \"\${dll_source_dir_vcpkg}/*.pdb\")
				list(LENGTH _vcpkg_pdbs _num_vcpkg_pdbs)
				if (_num_vcpkg_pdbs GREATER 0)
					foreach(_vcpkg_pdb IN LISTS _vcpkg_pdbs)
						get_filename_component(_vcpkg_pdb_filename \"\${_vcpkg_pdb}\" NAME)
						execute_process(COMMAND \${CMAKE_COMMAND} -E echo \"++install CODE: Copying PDB file: \${_vcpkg_pdb_filename}\")
						file(COPY_FILE \"\${_vcpkg_pdb}\" \"\${CMAKE_INSTALL_PREFIX}/${WZ_APP_INSTALL_DEST}/\${_vcpkg_pdb_filename}\")
					endforeach()
				else()
					message(WARNING \"Did not find any .pdb files in: \${dll_source_dir_vcpkg}\")
				endif()
			" COMPONENT AdditionalDebugSymbols)

			# Install LibANGLE DLLs (if available, from vcpkg)
			find_package(unofficial-angle CONFIG)
			if(unofficial-angle_FOUND)
				if(TARGET unofficial::angle::libEGL)
					get_target_property(_libEGL_dll unofficial::angle::libEGL IMPORTED_LOCATION_RELEASE)
				endif()

				if(TARGET unofficial::angle::libGLESv2)
					get_target_property(_libGLESv2_dll unofficial::angle::libGLESv2 IMPORTED_LOCATION_RELEASE)
				endif()

				if(_libEGL_dll AND _libGLESv2_dll)
					if(EXISTS "${_libEGL_dll}")
						message( STATUS "LibANGLE (libEGL): ${_libEGL_dll}" )
						install(FILES
							"${_libEGL_dll}"
							COMPONENT Core DESTINATION "${WZ_APP_INSTALL_DEST}"
							RENAME "libEGL.dll" # Make sure it has the proper name (which SDL2 expects)
						)
					else()
						message( WARNING "Missing expected libEGL DLL at: ${_libEGL_dll}" )
					endif()
					if(EXISTS "${_libGLESv2_dll}")
						message( STATUS "LibANGLE (libGLESv2): ${_libGLESv2_dll}" )
						install(FILES
							"${_libGLESv2_dll}"
							COMPONENT Core DESTINATION "${WZ_APP_INSTALL_DEST}"
							RENAME "libGLESv2.dll" # Make sure it has the proper name (which SDL2 expects)
						)
					else()
						message( WARNING "Missing expected libGLESv2 DLL at: ${_libGLESv2_dll}" )
					endif()
				else()
					message( WARNING "Found LibANGLE, but could not find the built DLLs - LibANGLE will *not* be installed." )
				endif()
			else()
				message( WARNING "Could not find LibANGLE - LibANGLE will *not* be installed. (DirectX via LibANGLE will not be supported)" )
			endif()

		else()
			message( STATUS "CMAKE_CROSSCOMPILING is defined - skipping BundleUtilities" )
		endif()

		if(WZ_WIN_HAS_PDB)
			# Must install the PDB file or crash dumps won't be as useful
			install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_mainexename}.pdb" COMPONENT DebugSymbols DESTINATION "${WZ_APP_INSTALL_DEST}")
		endif()
	else()
		message( WARNING "Unable to get OUTPUT_NAME from warzone2100 target" )
	endif()
endif()
