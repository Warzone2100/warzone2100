cmake_minimum_required (VERSION 3.5)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/LICENSE"
	"${CMAKE_CURRENT_BINARY_DIR}/DejaVu/DejaVu.LICENSE.txt"
	COPYONLY
)

set(wz2100_fonts_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans-Bold.ttf"
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans.ttf"
	"${CMAKE_CURRENT_BINARY_DIR}/DejaVu/DejaVu.LICENSE.txt"
)

install(FILES
	${wz2100_fonts_FILES}
	COMPONENT Data DESTINATION "fonts"
)

set(wz2100_fonts_FILES ${wz2100_fonts_FILES} PARENT_SCOPE)
