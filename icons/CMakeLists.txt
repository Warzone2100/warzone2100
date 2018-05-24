cmake_minimum_required(VERSION 3.5)

if(UNIX AND NOT APPLE AND NOT WIN32)
	include(GNUInstallDirs)

	install(FILES warzone2100.desktop
		DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
		COMPONENT Core
	)
	install(FILES warzone2100.appdata.xml
		DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
		COMPONENT Core
	)
	install(FILES warzone2100.png
		DESTINATION ${CMAKE_INSTALL_DATADIR}/icons
		COMPONENT Core
	)
endif()
