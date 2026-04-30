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
	"jpeg_util.h"
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
	"jpeg_util.cpp"
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
	if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
		target_compile_definitions(ivis-opengl PUBLIC "$<$<CONFIG:Debug>:WZ_DEBUG_GFX_API_LEAKS>")
	else()
		target_compile_definitions(ivis-opengl PUBLIC "-DWZ_DEBUG_GFX_API_LEAKS")
	endif()
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
	set(_ios_moltenvk_vulkan_headers "${CMAKE_SOURCE_DIR}/platforms/ios/vendor/MoltenVK/include")
	if(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND EXISTS "${_ios_moltenvk_vulkan_headers}/vulkan/vulkan.h")
		set(VulkanHeaders_FOUND TRUE)
		set(VulkanHeaders_INCLUDE_DIRS "${_ios_moltenvk_vulkan_headers}")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND EXISTS "/usr/local/include/vulkan/vulkan.h")
		set(VulkanHeaders_FOUND TRUE)
		set(VulkanHeaders_INCLUDE_DIRS "/usr/local/include")
	else()
		find_package(VulkanHeaders 290) # minimum supported version of the Vulkan headers is 290
	endif()
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

# Unfortunately, some distribution packages seem to have broken CMake Config files for libjpeg-turbo, which simply don't work
# Default to using the vendored version, but provide an option for those building who wish to use the system library version
set(_DEFAULT_WZ_USE_SYSTEM_LIBJPEG_TURBO_VALUE OFF)
if (VCPKG_TOOLCHAIN)
	set(_DEFAULT_WZ_USE_SYSTEM_LIBJPEG_TURBO_VALUE ON)
endif()
option(WZ_USE_SYSTEM_LIBJPEG_TURBO "Build with the system libjpeg-turbo library" ${_DEFAULT_WZ_USE_SYSTEM_LIBJPEG_TURBO_VALUE})

if (WZ_USE_SYSTEM_LIBJPEG_TURBO)
	find_package(libjpeg-turbo CONFIG REQUIRED)
	target_link_libraries(ivis-opengl PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)

else()
	include(ExternalProject)

	set(LIBJPEG_TURBO_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turboinstall")
	set(LIBJPEG_TURBO_INCLUDE_DIR "${LIBJPEG_TURBO_INSTALL_DIR}/include")
	set(LIBJPEG_TURBO_LIB_PATH "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX}")

	set(_additional_ext_proj_cmake_args)
	if (CMAKE_TOOLCHAIN_FILE)
		if(IS_ABSOLUTE "${CMAKE_TOOLCHAIN_FILE}")
			set(_absolute_cmake_toolchain_file "${CMAKE_TOOLCHAIN_FILE}")
		else()
			get_filename_component(_absolute_cmake_toolchain_file "${CMAKE_SOURCE_DIR}/${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
		endif()
		set(_additional_ext_proj_cmake_args "-DCMAKE_TOOLCHAIN_FILE=${_absolute_cmake_toolchain_file}")
	endif()
	if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
		set(LIBJPEG_TURBO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libjpeg-turbo")
		set(LIBJPEG_TURBO_BUILD_ARM64_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turbo-arm64")
		set(LIBJPEG_TURBO_BUILD_ARM64E_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turbo-arm64e")
		set(LIBJPEG_TURBO_INSTALL_ARM64_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turboinstall-arm64")
		set(LIBJPEG_TURBO_INSTALL_ARM64E_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turboinstall-arm64e")

		if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
			add_custom_command(
				OUTPUT "${LIBJPEG_TURBO_LIB_PATH}"
				COMMAND ${CMAKE_COMMAND} -E rm -rf "${LIBJPEG_TURBO_BUILD_ARM64_DIR}" "${LIBJPEG_TURBO_INSTALL_ARM64_DIR}" "${LIBJPEG_TURBO_INSTALL_DIR}"
				COMMAND ${CMAKE_COMMAND}
					"-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_ARM64_DIR}"
					-DWITH_TOOLS=OFF
					-DENABLE_STATIC=ON
					-DENABLE_SHARED=OFF
					"-DCMAKE_INSTALL_DEFAULT_LIBDIR=lib"
					"-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}"
					"-DCMAKE_SYSTEM_PROCESSOR=arm64"
					"-DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}"
					"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
					"-DCMAKE_OSX_ARCHITECTURES=arm64"
					"-DWITH_SIMD=OFF"
					"-DNEON_INTRINSICS=OFF"
					${_additional_ext_proj_cmake_args}
					-GXcode
					-S "${LIBJPEG_TURBO_SOURCE_DIR}"
					-B "${LIBJPEG_TURBO_BUILD_ARM64_DIR}"
				COMMAND ${CMAKE_COMMAND} --build "${LIBJPEG_TURBO_BUILD_ARM64_DIR}" --config Release --target install
				COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBJPEG_TURBO_INCLUDE_DIR}" "${LIBJPEG_TURBO_INSTALL_DIR}/lib"
				COMMAND ${CMAKE_COMMAND} -E copy_directory "${LIBJPEG_TURBO_INSTALL_ARM64_DIR}/include" "${LIBJPEG_TURBO_INCLUDE_DIR}"
				COMMAND ${CMAKE_COMMAND} -E copy
					"${LIBJPEG_TURBO_INSTALL_ARM64_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX}"
					"${LIBJPEG_TURBO_LIB_PATH}"
				DEPENDS "${LIBJPEG_TURBO_SOURCE_DIR}/CMakeLists.txt"
				VERBATIM
			)
		else()
			add_custom_command(
				OUTPUT "${LIBJPEG_TURBO_LIB_PATH}"
				COMMAND ${CMAKE_COMMAND} -E rm -rf "${LIBJPEG_TURBO_BUILD_ARM64_DIR}" "${LIBJPEG_TURBO_BUILD_ARM64E_DIR}" "${LIBJPEG_TURBO_INSTALL_ARM64_DIR}" "${LIBJPEG_TURBO_INSTALL_ARM64E_DIR}" "${LIBJPEG_TURBO_INSTALL_DIR}"
				COMMAND ${CMAKE_COMMAND}
					"-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_ARM64_DIR}"
					-DWITH_TOOLS=OFF
					-DENABLE_STATIC=ON
					-DENABLE_SHARED=OFF
					"-DCMAKE_INSTALL_DEFAULT_LIBDIR=lib"
					"-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}"
					"-DCMAKE_SYSTEM_PROCESSOR=arm64"
					"-DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}"
					"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
					"-DCMAKE_OSX_ARCHITECTURES=arm64"
					"-DWITH_SIMD=OFF"
					"-DNEON_INTRINSICS=OFF"
					${_additional_ext_proj_cmake_args}
					-GXcode
					-S "${LIBJPEG_TURBO_SOURCE_DIR}"
					-B "${LIBJPEG_TURBO_BUILD_ARM64_DIR}"
				COMMAND ${CMAKE_COMMAND} --build "${LIBJPEG_TURBO_BUILD_ARM64_DIR}" --config Release --target install
				COMMAND ${CMAKE_COMMAND}
					"-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_ARM64E_DIR}"
					-DWITH_TOOLS=OFF
					-DENABLE_STATIC=ON
					-DENABLE_SHARED=OFF
					"-DCMAKE_INSTALL_DEFAULT_LIBDIR=lib"
					"-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}"
					"-DCMAKE_SYSTEM_PROCESSOR=arm64e"
					"-DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}"
					"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
					"-DCMAKE_OSX_ARCHITECTURES=arm64e"
					"-DWITH_SIMD=OFF"
					"-DNEON_INTRINSICS=OFF"
					${_additional_ext_proj_cmake_args}
					-GXcode
					-S "${LIBJPEG_TURBO_SOURCE_DIR}"
					-B "${LIBJPEG_TURBO_BUILD_ARM64E_DIR}"
				COMMAND ${CMAKE_COMMAND} --build "${LIBJPEG_TURBO_BUILD_ARM64E_DIR}" --config Release --target install
				COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBJPEG_TURBO_INCLUDE_DIR}" "${LIBJPEG_TURBO_INSTALL_DIR}/lib"
				COMMAND ${CMAKE_COMMAND} -E copy_directory "${LIBJPEG_TURBO_INSTALL_ARM64_DIR}/include" "${LIBJPEG_TURBO_INCLUDE_DIR}"
				COMMAND xcrun --sdk iphoneos lipo -create
					"${LIBJPEG_TURBO_INSTALL_ARM64_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX}"
					"${LIBJPEG_TURBO_INSTALL_ARM64E_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}turbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX}"
					-output "${LIBJPEG_TURBO_LIB_PATH}"
				DEPENDS "${LIBJPEG_TURBO_SOURCE_DIR}/CMakeLists.txt"
				VERBATIM
			)
		endif()
		add_custom_target(libjpeg-turbo-ext DEPENDS "${LIBJPEG_TURBO_LIB_PATH}")
	else()
		ExternalProject_Add(libjpeg-turbo-ext
			SOURCE_DIR		"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libjpeg-turbo"
			BINARY_DIR		"${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libjpeg-turbo"
			INSTALL_DIR		"${LIBJPEG_TURBO_INSTALL_DIR}"
			# DOWNLOAD_COMMAND  ""  # Explicitly skip download for local sources
			CMAKE_ARGS		"-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" -DWITH_TOOLS=OFF -DENABLE_STATIC=ON -DENABLE_SHARED=OFF "-DCMAKE_INSTALL_DEFAULT_LIBDIR=lib" ${_additional_ext_proj_cmake_args}
			BUILD_BYPRODUCTS "${LIBJPEG_TURBO_LIB_PATH}"
		)
	endif()

	# Create the imported library
	add_library(libjpeg-turbo::turbojpeg-static STATIC IMPORTED GLOBAL)
	add_dependencies(libjpeg-turbo::turbojpeg-static libjpeg-turbo-ext)

	# Set the library path and include directory

	# This is a workaround for the fact that included directories of an imported
	# target should exist in the filesystem already at the configuration time.
	# ref: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
	file(MAKE_DIRECTORY "${LIBJPEG_TURBO_INCLUDE_DIR}")

	set_target_properties(libjpeg-turbo::turbojpeg-static PROPERTIES
		IMPORTED_LOCATION "${LIBJPEG_TURBO_LIB_PATH}"
		INTERFACE_INCLUDE_DIRECTORIES "${LIBJPEG_TURBO_INCLUDE_DIR}"
	)

	add_dependencies(ivis-opengl libjpeg-turbo-ext) # Ensure libjpeg-turbo builds first
	target_link_libraries(ivis-opengl PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)
endif()
