file(GLOB HEADERS
	"bitimage.h"
	"gfx_api.h"
	"gfx_api_gl.h"
	"gfx_api_null.h"
	"gfx_api_vk.h"
	"imd.h"
	"ivisdef.h"
	"jpeg_encoder.h"
	"pieblitfunc.h"
	"pieclip.h"
	"piedef.h"
	"piefunc.h"
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
	"gfx_api.cpp"
	"gfx_api_gl.cpp"
	"gfx_api_null.cpp"
	"gfx_api_vk.cpp"
	"imdload.cpp"
	"jpeg_encoder.cpp"
	"pieblitfunc.cpp"
	"pieclip.cpp"
	"piedraw.cpp"
	"piefunc.cpp"
	"piematrix.cpp"
	"piemode.cpp"
	"piepalette.cpp"
	"piestate.cpp"
	"png_util.cpp"
	"screen.cpp"
	"tex.cpp"
	"textdraw.cpp"
	"3rdparty/stb_image_resize.cpp"
)

find_package(PNG 1.2 REQUIRED)
find_package(Freetype REQUIRED)
find_package(Harfbuzz 1.0 REQUIRED)

include(CheckCXXCompilerFlag)

add_library(ivis-opengl STATIC ${HEADERS} ${SRC})
set_property(TARGET ivis-opengl PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(ivis-opengl)
target_include_directories(ivis-opengl PRIVATE ${HARFBUZZ_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_ft2build})
target_link_libraries(ivis-opengl PRIVATE framework PNG::PNG ${HARFBUZZ_LIBRARIES} ${FREETYPE_LIBRARIES})
target_link_libraries(ivis-opengl PUBLIC glad)
if(WZ_ENABLE_BACKEND_VULKAN)
	find_package(VulkanHeaders 148) # minimum supported version of the Vulkan headers is 148
	set(_vulkanheaders_dl_gittag "sdk-1.2.162.0")
	if((NOT VulkanHeaders_FOUND AND NOT ${CMAKE_VERSION} VERSION_LESS "3.11.0") AND (NOT WZ_DISABLE_FETCHCONTENT_GIT_CLONE))
		# Fetch newer Vulkan headers (using FetchContent, which requires CMake 3.11+)
		include(FetchContent)
		FetchContent_Declare(
			vulkan_headers
			GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
			GIT_TAG        ${_vulkanheaders_dl_gittag}
		)
		FetchContent_GetProperties(vulkan_headers)
		if(NOT vulkan_headers_POPULATED)
			FetchContent_Populate(vulkan_headers)
		endif()
		set(VulkanHeaders_FOUND TRUE)
		set(VulkanHeaders_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
		message(STATUS "Using fetched Vulkan headers: ${VulkanHeaders_INCLUDE_DIRS}")
	elseif((NOT VulkanHeaders_FOUND AND ${CMAKE_VERSION} VERSION_LESS "3.11.0") AND (NOT WZ_DISABLE_FETCHCONTENT_GIT_CLONE))
		# Fetch newer Vulkan headers using Git directly (on CMake < 3.11)
		find_package(Git QUIET)
		if(Git_FOUND)
			if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/vulkan_headers" AND IS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vulkan_headers")
				# Because this branch is only executed for CMake < 3.11, use "remove_directory"
				execute_process(
					COMMAND ${CMAKE_COMMAND} "-E" "remove_directory" "vulkan_headers"
					WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
			endif()
			execute_process(
				COMMAND ${GIT_EXECUTABLE} clone https://github.com/KhronosGroup/Vulkan-Headers.git "vulkan_headers"
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				RESULT_VARIABLE _git_exstatus
				ERROR_QUIET)
			if(_git_exstatus EQUAL 0)
				execute_process(
					COMMAND ${GIT_EXECUTABLE} checkout "tags/${_vulkanheaders_dl_gittag}" -b version_commit
					WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vulkan_headers"
					RESULT_VARIABLE _git_exstatus
					ERROR_QUIET)
			endif()
			if(_git_exstatus EQUAL 0)
				set(VulkanHeaders_FOUND TRUE)
				set(VulkanHeaders_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/vulkan_headers/include")
				message(STATUS "Using (Git) fetched Vulkan headers: ${VulkanHeaders_INCLUDE_DIRS}")
			endif()
		endif()
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

