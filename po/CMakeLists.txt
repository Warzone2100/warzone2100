project(translation)

file(GLOB pofiles "*.po")

set(PO_FILES_LIST "")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	set(MSG_FMT "msgfmt.exe")
else()
	find_program(MSG_FMT "msgfmt")
	foreach(f ${pofiles})
		get_filename_component(stripped_f ${f} NAME_WE)
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/")
		add_custom_command(
			OUTPUT "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo"
			COMMAND ${MSG_FMT} "-c" "-o" "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo" "${f}"
			DEPENDS ${f}
		)
		list(APPEND PO_FILES_LIST "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo")
		install(FILES "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo" DESTINATION
		"locale/${stripped_f}/LC_MESSAGES/")
	endforeach()
endif()

add_custom_target(generate_mo ALL
	DEPENDS ${PO_FILES_LIST}
	)
