cmake_minimum_required (VERSION 3.5)

if(NOT DEFINED WZ_DATADIR)
	message(FATAL_ERROR "This file should be included in the project's root CMakeLists.txt, after WZ_DATADIR is defined.")
endif()

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
	COMPONENT Fonts DESTINATION "${WZ_DATADIR}/fonts"
)

set(wz2100_fonts_FILES ${wz2100_fonts_FILES} PARENT_SCOPE)
