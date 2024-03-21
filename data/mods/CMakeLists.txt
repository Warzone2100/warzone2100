if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included after WZ_DATADIR is defined.")
endif()

OPTION(WZ_BUILTIN_MODS_CAMCLASSIC "Include the classic campaign balance mod" ON)

# Note: Always package mods as actual zip files, *even for Emscripten builds*
find_package(ZIP REQUIRED)

####################################################

# IMPORTANT: Must set GENERATED property at this directory level for autorevision.h
set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)

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

	if(NOT TARGET mod_info_gen)
		add_custom_target(mod_info_gen)
		set_property(TARGET mod_info_gen PROPERTY FOLDER "_WZBuildProcessTargets")
	endif()

	_MODINFO_JSON_GET_UNIQUE_TARGET_NAME(modinfo uniqueTargetName)

	add_custom_target(${uniqueTargetName} ALL
		DEPENDS "${OUTPUT_PATH}"
	)
	set_property(TARGET ${uniqueTargetName} PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(${uniqueTargetName} autorevision) # Ensure ordering and non-concurrency

	add_dependencies(mod_info_gen ${uniqueTargetName})

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

####################################################

set(wz2100_mods_FILES ${wz2100_mods_FILES} PARENT_SCOPE)
