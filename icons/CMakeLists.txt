cmake_minimum_required(VERSION 3.5)

if(UNIX AND NOT APPLE AND NOT WIN32)
	include(GNUInstallDirs)

	configure_file("warzone2100.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}.desktop" @ONLY)
	configure_file("warzone2100.appdata.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}.appdata.xml" @ONLY)

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		COMPONENT Core
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/warzone2100${WZ_OUTPUT_NAME_SUFFIX}.appdata.xml"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
		COMPONENT Core
	)
	install(FILES warzone2100.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
		RENAME "warzone2100${WZ_OUTPUT_NAME_SUFFIX}.png"
		COMPONENT Core
	)
endif()
