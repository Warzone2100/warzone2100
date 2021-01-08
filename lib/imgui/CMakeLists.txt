cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS
	"imgui.h"
	"imconfig.h"
	"imgui_impl_opengl3.h"
	"imgui_impl_sdl.h"
	"imgui_internal.h"
	"imstb_rectpack.h"
	"imstb_textedit.h"
	"imstb_truetype.h"
)

file(GLOB SRC
	"imgui.cpp"
	"imgui_demo.cpp"
	"imgui_draw.cpp"
	"imgui_impl_opengl3.cpp"
	"imgui_impl_sdl.cpp"
	"imgui_widgets.cpp"
)

find_package(OpenGL REQUIRED)
find_package(GLEW 1.5.2 REQUIRED)

add_library(imgui ${HEADERS} ${SRC})
set_property(TARGET imgui PROPERTY FOLDER "lib")
target_link_libraries(imgui PUBLIC ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} GLEW::GLEW)
target_link_libraries(imgui PRIVATE glad ivis-opengl)
