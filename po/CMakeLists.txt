project(translation)

add_custom_target(generate_mo)

file(GLOB pofiles "*.po")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	set(MSG_FMT "msgfmt.exe")
else()
	set(MSG_FMT "msgfmt")
endif()

foreach(f ${pofiles})
	get_filename_component(stripped_f ${f} NAME_WE)
	add_custom_command(
		TARGET generate_mo
		COMMAND ${MSG_FMT}
		ARGS "-c" "-o" "${CMAKE_CURRENT_SOURCE_DIR}/${stripped_f}.gmo" "${f}"
	)
endforeach()

add_dependencies(warzone2100 generate_mo)
