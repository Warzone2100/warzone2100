if(UNIX AND NOT APPLE AND NOT WIN32 AND NOT EMSCRIPTEN)
	include(GNUInstallDirs)

	if (NOT DEFINED WZ_APPSTREAM_ID)
		set(WZ_APPSTREAM_ID "net.wz2100.warzone2100")
	endif()

	############################
	# Generate metainfo.xml (w/ autorevision)

	# IMPORTANT: Must set GENERATED property at this directory level for autorevision.h
	set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
	set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)

	# Generate the metainfo.xml info (with version info)
	set(_metainfo_xml_template_file "${CMAKE_CURRENT_SOURCE_DIR}/net.wz2100.warzone2100.metainfo.xml.in")
	set(_metainfo_xml_output_file "${CMAKE_CURRENT_BINARY_DIR}/${WZ_APPSTREAM_ID}${WZ_OUTPUT_NAME_SUFFIX}.metainfo.xml")
	add_custom_command(
		OUTPUT "${_metainfo_xml_output_file}"
		COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_metainfo_xml_template_file} -DOUTPUT_FILE=${_metainfo_xml_output_file} -DWZ_OUTPUT_NAME_SUFFIX=${WZ_OUTPUT_NAME_SUFFIX} -DWZ_NAME_SUFFIX=${WZ_NAME_SUFFIX} -DWZ_APPSTREAM_ID=${WZ_APPSTREAM_ID} -P ${CMAKE_CURRENT_SOURCE_DIR}/autorevision_metainfo.cmake
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS "${_metainfo_xml_template_file}" "${wz2100_autorevision_cache_file}" "${CMAKE_CURRENT_SOURCE_DIR}/autorevision_metainfo.cmake"
		VERBATIM
	)

	add_custom_target(autorevision_metainfo ALL
		DEPENDS
			"${_metainfo_xml_output_file}"
	)
	set_property(TARGET autorevision_metainfo PROPERTY FOLDER "_WZBuildProcessTargets")
	add_dependencies(autorevision_metainfo autorevision) # Ensure ordering and non-concurrency

	############################
	# Generate .desktop file

	if (NOT WZ_SKIP_OUTPUT_NAME_SUFFIX_ON_BINARY)
		set(WZ_OUTPUT_BINARY_SUFFIX "${WZ_OUTPUT_NAME_SUFFIX}")
	else()
		unset(WZ_OUTPUT_BINARY_SUFFIX)
	endif()

	configure_file("net.wz2100.warzone2100.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/${WZ_APPSTREAM_ID}${WZ_OUTPUT_NAME_SUFFIX}.desktop" @ONLY)

	############################
	# Install both

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${WZ_APPSTREAM_ID}${WZ_OUTPUT_NAME_SUFFIX}.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		COMPONENT Core
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${WZ_APPSTREAM_ID}${WZ_OUTPUT_NAME_SUFFIX}.metainfo.xml"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
		COMPONENT Core
	)
	install(FILES "${PROJECT_SOURCE_DIR}/icons/warzone2100.png"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
		RENAME "${WZ_APPSTREAM_ID}${WZ_OUTPUT_NAME_SUFFIX}.png"
		COMPONENT Core
	)
endif()
