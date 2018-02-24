cmake_minimum_required (VERSION 3.5)

install(FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans-Bold.ttf"
		"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans.ttf"
		COMPONENT Core DESTINATION "fonts"
)

install(FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/LICENSE"
		RENAME "DejaVu.LICENSE.txt"
		COMPONENT Core DESTINATION "fonts"
)
