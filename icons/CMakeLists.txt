if(UNIX AND NOT APPLE AND NOT WIN32)
	include(GNUInstallDirs)

	configure_file("net.wz2100.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/net.wz2100.desktop" @ONLY)
	configure_file("net.wz2100.metainfo.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/net.wz2100.metainfo.xml" @ONLY)

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/net.wz2100.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
		COMPONENT Core
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/net.wz2100.metainfo.xml"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
		COMPONENT Core
	)
	install(FILES warzone2100.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
		RENAME "net.wz2100.png"
		COMPONENT Core
	)
endif()
