if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(WZ_USE_SPNG TRUE)
endif()

set(_png_util_cpp "png_util_libpng.cpp")
if(WZ_USE_SPNG)
	set(_png_util_cpp "png_util_spng.cpp")
endif()

file(GLOB HEADERS
	"bitimage.h"
	"culling.h"
	"gfx_api.h"
	"gfx_api_formats_def.h"
	"gfx_api_gl.h"
	"gfx_api_image_basis_priv.h"
	"gfx_api_image_compress_priv.h"
	"gfx_api_null.h"
	"gfx_api_vk.h"
	"imd.h"
	"ivisdef.h"
	"jpeg_encoder.h"
	"pieblitfunc.h"
	"pieclip.h"
	"piedef.h"
	"piefunc.h"
	"pielighting.h"
	"piematrix.h"
	"piemode.h"
	"pienormalize.h"
	"piepalette.h"
	"piestate.h"
	"pietypes.h"
	"png_util.h"
	"screen.h"
	"tex.h"
	"textdraw.h"
	"3rdparty/stb_image_resize.h"
)

file(GLOB SRC
	"bitimage.cpp"
	"culling.cpp"
	"gfx_api.cpp"
	"gfx_api_gl.cpp"
	"gfx_api_image_basis_priv.cpp"
	"gfx_api_image_compress_priv.cpp"
	"gfx_api_null.cpp"
	"gfx_api_vk.cpp"
	"imdload.cpp"
	"jpeg_encoder.cpp"
	"pieblitfunc.cpp"
	"pieclip.cpp"
	"piedraw.cpp"
	"piefunc.cpp"
	"pieimage.cpp"
	"pielighting.cpp"
	"piematrix.cpp"
	"piemode.cpp"
	"piepalette.cpp"
	"piestate.cpp"
	"${_png_util_cpp}"
	"screen.cpp"
	"tex.cpp"
	"textdraw.cpp"
	"3rdparty/stb_image_resize.cpp"
)

add_subdirectory(3rdparty/etcpak)

if(WZ_USE_SPNG)
	find_package(SPNG CONFIG REQUIRED)
else()
	find_package(PNG 1.2 REQUIRED)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	# We should be using the Emscripten port linker flags for FreeType & Harfbuzz
else()
	find_package(Freetype REQUIRED)
	find_package(Harfbuzz 1.0 REQUIRED)
endif()
find_package(Fribidi) # recommended, but optional

include(CheckCXXCompilerFlag)

add_library(ivis-opengl STATIC ${HEADERS} ${SRC})
set_property(TARGET ivis-opengl PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(ivis-opengl)

target_link_libraries(ivis-opengl PRIVATE framework launchinfo fmt::fmt)
if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	# We should be using the Emscripten port linker flags for FreeType & Harfbuzz
else()
	target_include_directories(ivis-opengl PRIVATE ${HARFBUZZ_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_ft2build})
	target_link_libraries(ivis-opengl PRIVATE ${HARFBUZZ_LIBRARIES} ${FREETYPE_LIBRARIES})
endif()
if(WZ_USE_SPNG)
	target_link_libraries(ivis-opengl PRIVATE $<IF:$<TARGET_EXISTS:spng::spng>,spng::spng,spng::spng_static>)
else()
	target_link_libraries(ivis-opengl PRIVATE PNG::PNG)
endif()
target_link_libraries(ivis-opengl PUBLIC glad)
target_link_libraries(ivis-opengl PUBLIC optional-lite)
if (WZ_DEBUG_GFX_API_LEAKS)
	target_compile_definitions(ivis-opengl PUBLIC "-DWZ_DEBUG_GFX_API_LEAKS")
endif()
if (Fribidi_FOUND)
	target_include_directories(ivis-opengl PRIVATE ${FRIBIDI_INCLUDE_DIRS})
	target_link_libraries(ivis-opengl PRIVATE ${FRIBIDI_LIBRARIES})
	target_compile_definitions(ivis-opengl PRIVATE "-DWZ_FRIBIDI_ENABLED")
else()
	message(WARNING "Fribidi not found - WZ will be compiled *without* Fribidi support")
endif()
if(TARGET etcpak)
	target_link_libraries(ivis-opengl PRIVATE etcpak)
	target_compile_definitions(ivis-opengl PRIVATE "-DETCPAK_ENABLED")
endif()
if(WZ_ENABLE_BASIS_UNIVERSAL AND TARGET basis_transcoder)
	target_link_libraries(ivis-opengl PRIVATE basis_transcoder)
	target_compile_definitions(ivis-opengl PRIVATE "-DBASIS_ENABLED")
else()
	message(WARNING "Basis-universal support is disabled")
endif()

if(WZ_ENABLE_BACKEND_VULKAN)
	find_package(VulkanHeaders 290) # minimum supported version of the Vulkan headers is 290
	set(_vulkanheaders_dl_gittag "vulkan-sdk-1.3.296.0")
	if(NOT VulkanHeaders_FOUND AND (NOT WZ_DISABLE_FETCHCONTENT_GIT_CLONE))
		# Fetch newer Vulkan headers (using FetchContent, which requires CMake 3.11+)
		include(FetchContent)
		FetchContent_Declare(
			vulkan_headers
			GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
			GIT_TAG        ${_vulkanheaders_dl_gittag}
			# Intentionally do not use the Vulkan-Headers build system
			# Reference: https://discourse.cmake.org/t/prevent-fetchcontent-makeavailable-to-execute-cmakelists-txt/12704/3
			SOURCE_SUBDIR dummy
		)
		FetchContent_MakeAvailable(vulkan_headers)
		set(VulkanHeaders_FOUND TRUE)
		set(VulkanHeaders_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
		message(STATUS "Using fetched Vulkan headers: ${VulkanHeaders_INCLUDE_DIRS}")
	endif()
	if(VulkanHeaders_FOUND AND TARGET glsl_compilation)
		target_compile_definitions(ivis-opengl PUBLIC "-DWZ_VULKAN_ENABLED")
		message(STATUS "@@ Using VulkanHeaders_INCLUDE_DIRS=${VulkanHeaders_INCLUDE_DIRS}")
		target_include_directories(ivis-opengl PUBLIC ${VulkanHeaders_INCLUDE_DIRS}) # For now, make the include path public so the backend (ex. SDL) can access type declarations
		target_sources(ivis-opengl PRIVATE "3rdparty/vkh_renderpasscompat.cpp" "3rdparty/vkh_renderpasscompat.hpp")
		target_sources(ivis-opengl PRIVATE "3rdparty/vkh_info.cpp" "3rdparty/vkh_info.hpp")

		target_sources(ivis-opengl PRIVATE "3rdparty/vk_mem_alloc.cpp" "3rdparty/vk_mem_alloc.h")
		CHECK_CXX_COMPILER_FLAG(-Wno-pedantic COMPILER_SUPPORTS_WNO_PEDANTIC)
		if(COMPILER_SUPPORTS_WNO_PEDANTIC)
			set_source_files_properties("3rdparty/vk_mem_alloc.cpp" PROPERTIES COMPILE_FLAGS -Wno-pedantic)
		endif()

		# Do *NOT* link to the Vulkan library - should be loaded dynamically

		add_dependencies(ivis-opengl glsl_compilation)
	else()
		if(NOT VulkanHeaders_FOUND)
			message(STATUS "Vulkan headers not found - disabling Vulkan backend")
		elseif(NOT TARGET glsl_compilation)
			message(STATUS "GLSLC shader compiler not found - disabling Vulkan backend")
		endif()
	endif()
endif()

