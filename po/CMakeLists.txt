project(translation)

add_custom_target(generate_mo ALL)

file(GLOB pofiles "*.po")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	set(MSG_FMT "msgfmt.exe")
else()
	set(MSG_FMT "msgfmt")
endif()

foreach(f ${pofiles})
	get_filename_component(stripped_f ${f} NAME_WE)
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/")
	add_custom_command(
		TARGET generate_mo
		COMMAND ${MSG_FMT}
		ARGS "-c" "-o" "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo" "${f}"
	)
	install(FILES "${CMAKE_BINARY_DIR}/locale/${stripped_f}/LC_MESSAGES/warzone2100.gmo" DESTINATION
	"locale/${stripped_f}/LC_MESSAGES/")
endforeach()
