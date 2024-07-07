if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included after WZ_DATADIR is defined.")
endif()

set(_default_bundled_addon_mod_option_value ON)
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(_default_bundled_addon_mod_option_value OFF)
endif()

OPTION(WZ_BUILTIN_MODS_CAMCLASSIC "Include the classic campaign balance mod" ON)
OPTION(WZ_BUILTIN_MODS_RECLAMATION "Include the addon campaign: Reclamation" ${_default_bundled_addon_mod_option_value})
OPTION(WZ_BUILTIN_MODS_FRACTUREDKINGDOM "Include the addon campaign: Fractured Kingdom" ${_default_bundled_addon_mod_option_value})

# Note: Always package mods as actual zip files, *even for Emscripten builds*
find_package(ZIP REQUIRED)

####################################################

# IMPORTANT: Must set GENERATED property at this directory level for autorevision.h
set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)

function(_MODINFO_CREATE_BASE_TARGET)
	if(NOT TARGET mod_info_gen)
		add_custom_target(mod_info_gen)
		set_property(TARGET mod_info_gen PROPERTY FOLDER "_WZBuildProcessTargets")
	endif()
endfunction()

function(_MODINFO_JSON_GET_UNIQUE_TARGET_NAME _name _unique_name)
	set(propertyName "_MODINFO_JSON_UNIQUE_COUNTER_${_name}")
	get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
	if(NOT currentCounter)
		set(currentCounter 1)
	endif()
	set(${_unique_name} "${_name}_${currentCounter}" PARENT_SCOPE)
	math(EXPR currentCounter "${currentCounter} + 1")
	set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter} )
endfunction()

function(GENERATE_MOD_INFO_JSON TEMPLATE_FILE OUTPUT_PATH)

	get_filename_component(OUTPUT_PATH_DIR ${OUTPUT_PATH} DIRECTORY)
	if(NOT EXISTS "${OUTPUT_PATH_DIR}")
		file(MAKE_DIRECTORY "${OUTPUT_PATH_DIR}")
	endif()

	if(NOT EXISTS "${TEMPLATE_FILE}")
		message(FATAL_ERROR "Missing input template file: ${TEMPLATE_FILE}")
	endif()

	# Generate the mod-info.json (with version info)
	add_custom_command(
		OUTPUT "${OUTPUT_PATH}"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${TEMPLATE_FILE} -DOUTPUT_FILE=${OUTPUT_PATH} -P ${CMAKE_CURRENT_SOURCE_DIR}/autorevision_modinfo.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${TEMPLATE_FILE}" "${wz2100_autorevision_cache_file}" "${CMAKE_CURRENT_SOURCE_DIR}/autorevision_modinfo.cmake"
		VERBATIM
	)

	_MODINFO_CREATE_BASE_TARGET()
	_MODINFO_JSON_GET_UNIQUE_TARGET_NAME(modinfo uniqueTargetName)

	add_custom_target(${uniqueTargetName} ALL
		DEPENDS "${OUTPUT_PATH}"
	)
	set_property(TARGET ${uniqueTargetName} PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(${uniqueTargetName} autorevision) # Ensure ordering and non-concurrency

	add_dependencies(mod_info_gen ${uniqueTargetName})

endfunction()

function(PACKAGE_ADDON_CAMPAIGN INPUT_PATH OUTPUT_PATH)

	file(REMOVE "${OUTPUT_PATH}")

	get_filename_component(_input_folder_name "${INPUT_PATH}" NAME)

	if (NOT EXISTS "${INPUT_PATH}")
		message(FATAL_ERROR "Missing path: ${INPUT_PATH} - (did you remember to initialize git submodules?)")
		return()
	endif()

	file(RELATIVE_PATH relative_input_path "${CMAKE_CURRENT_SOURCE_DIR}" "${INPUT_PATH}")
	set(STAGING_FILES_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/staging/files/${relative_input_path}")
	set(STAGING_INFO_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/staging/info/${relative_input_path}")
	message(DEBUG "STAGING_FILES_DIRECTORY=${STAGING_FILES_DIRECTORY}")
	message(DEBUG "STAGING_INFO_DIRECTORY=${STAGING_INFO_DIRECTORY}")

	file(GLOB_RECURSE _mod_input_files_list LIST_DIRECTORIES true "${INPUT_PATH}/*")
	set(_output_files_list)
	set(_info_file_names)
	foreach(input_file IN LISTS _mod_input_files_list)
		get_filename_component(_input_file_name "${input_file}" NAME)
		file(RELATIVE_PATH _input_file_relative "${INPUT_PATH}" "${input_file}")
		if (_input_file_name MATCHES "^[.]") # ignore files starting with period
			message(DEBUG "Ignoring mod path: ${_input_file_name}")
			continue()
		endif()
		if (IS_DIRECTORY "${input_file}")
			list(APPEND _output_files_list "${STAGING_FILES_DIRECTORY}/${_input_file_relative}")
			continue()
		endif()
		# handling files:
		if ("${_input_file_relative}" MATCHES "^(mod-info.json|mod-banner.png)")
			# Special case - these end up in the STAGING_INFO_DIRECTORY directory
			list(APPEND _output_files_list "${STAGING_INFO_DIRECTORY}/${_input_file_relative}")
			list(APPEND _info_file_names "${_input_file_name}")
		else()
			list(APPEND _output_files_list "${STAGING_FILES_DIRECTORY}/${_input_file_relative}")
		endif()
	endforeach()

	file(GLOB _mod_root_paths LIST_DIRECTORIES true RELATIVE "${INPUT_PATH}" "${INPUT_PATH}/*")
	list(FILTER _mod_root_paths EXCLUDE REGEX "^[.]")
	list(FILTER _mod_root_paths EXCLUDE REGEX "^(mod-info.json|mod-banner.png)")

	# Stage the mod files
	add_custom_command(
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DINPUT_PATH=${INPUT_PATH} -DSTAGING_FILES_DIRECTORY=${STAGING_FILES_DIRECTORY} -DSTAGING_INFO_DIRECTORY=${STAGING_INFO_DIRECTORY} -P ${CMAKE_CURRENT_SOURCE_DIR}/stage_modfiles.cmake
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
		DEPENDS ${_mod_input_files_list} "${wz2100_autorevision_cache_file}" "${CMAKE_CURRENT_SOURCE_DIR}/stage_modfiles.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/autorevision_modinfo.cmake"
		OUTPUT ${_output_files_list}
		VERBATIM
	)
	_MODINFO_CREATE_BASE_TARGET()
	set(uniqueTargetName "staging_mods_campaign_${_input_folder_name}")
	add_custom_target(${uniqueTargetName} ALL
		DEPENDS ${_output_files_list}
	)
	set_property(TARGET ${uniqueTargetName} PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(${uniqueTargetName} autorevision) # Ensure ordering and non-concurrency
	add_dependencies(mod_info_gen ${uniqueTargetName})

	set(_target_name "data_mods_campaign_${_input_folder_name}")
	COMPRESS_ZIP(OUTPUT "${OUTPUT_PATH}"
		COMPRESSION_LEVEL 7
		PATHS
			${_info_file_names}
		WORKING_DIRECTORY "${STAGING_INFO_DIRECTORY}"
		PATHS
			${_mod_root_paths}
		WORKING_DIRECTORY "${STAGING_FILES_DIRECTORY}"
		BUILD_ALWAYS_TARGET "${_target_name}"
		IGNORE_GIT
		QUIET
	)
	add_dependencies("${_target_name}" mod_info_gen)
	set_property(TARGET "${_target_name}" PROPERTY FOLDER "data")
	add_dependencies(data_mods "${_target_name}")

	list(APPEND wz2100_mods_FILES "${OUTPUT_PATH}")
	set(wz2100_mods_FILES "${wz2100_mods_FILES}" PARENT_SCOPE)

endfunction()

add_custom_target(data_mods)
set_property(TARGET data_mods PROPERTY FOLDER "_WZBuildProcessTargets")

####################################################
# Campaign mods

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/campaign")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/campaign")
endif()

if (WZ_BUILTIN_MODS_CAMCLASSIC)

	# Classic campaign balance mod

	file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/campaign/wz2100_camclassic.wz")

	GENERATE_MOD_INFO_JSON(
		"${CMAKE_CURRENT_SOURCE_DIR}/campaign/wz2100_camclassic/mod-info.json.in"
		"${CMAKE_CURRENT_BINARY_DIR}/generated_info_json/wz2100_camclassic/mod-info.json"
	)

	COMPRESS_ZIP(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/campaign/wz2100_camclassic.wz"
		COMPRESSION_LEVEL 7
		PATHS
			"mod-info.json"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated_info_json/wz2100_camclassic"
		PATHS
			"stats"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/campaign/wz2100_camclassic"
		BUILD_ALWAYS_TARGET data_mods_campaign_wz2100_camclassic
		IGNORE_GIT
		QUIET
	)
	add_dependencies(data_mods_campaign_wz2100_camclassic mod_info_gen)
	set_property(TARGET data_mods_campaign_wz2100_camclassic PROPERTY FOLDER "data")
	list(APPEND wz2100_mods_FILES "${CMAKE_CURRENT_BINARY_DIR}/campaign/wz2100_camclassic.wz")
	add_dependencies(data_mods data_mods_campaign_wz2100_camclassic)

endif(WZ_BUILTIN_MODS_CAMCLASSIC)

if (WZ_BUILTIN_MODS_RECLAMATION)

	# Addon campaign: Reclamation
	PACKAGE_ADDON_CAMPAIGN("${CMAKE_CURRENT_SOURCE_DIR}/campaign/reclamation" "${CMAKE_CURRENT_BINARY_DIR}/campaign/reclamation.wz")

endif(WZ_BUILTIN_MODS_RECLAMATION)

if (WZ_BUILTIN_MODS_FRACTUREDKINGDOM)

	# Addon campaign: Fractured Kingdom
	PACKAGE_ADDON_CAMPAIGN("${CMAKE_CURRENT_SOURCE_DIR}/campaign/fractured-kingdom" "${CMAKE_CURRENT_BINARY_DIR}/campaign/fractured_kingdom.wz")

endif(WZ_BUILTIN_MODS_FRACTUREDKINGDOM)

####################################################

set(wz2100_mods_FILES ${wz2100_mods_FILES} PARENT_SCOPE)
