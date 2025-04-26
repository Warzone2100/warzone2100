############################
# Versioning (autorevision)

# IMPORTANT: Must set GENERATED property at this directory level for autorevision.h
set_source_files_properties("${wz2100_autorevision_cache_file}" PROPERTIES GENERATED TRUE)
set_source_files_properties("${wz2100_autorevision_h_file}" PROPERTIES GENERATED TRUE)

# Generate the netplay_config.h NETCODE_VERSION info
set(_netplay_config_template_file "${CMAKE_CURRENT_SOURCE_DIR}/netplay_config.h.in")
set(_netplay_config_output_file "${CMAKE_CURRENT_BINARY_DIR}/netplay_config.h")
add_custom_command(
	OUTPUT "${_netplay_config_output_file}"
	COMMAND ${CMAKE_COMMAND} -DCACHEFILE=${wz2100_autorevision_cache_file} -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DTEMPLATE_FILE=${_netplay_config_template_file} -DOUTPUT_FILE=${_netplay_config_output_file} -P ${CMAKE_CURRENT_SOURCE_DIR}/autorevision_netplay.cmake
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	DEPENDS "${_netplay_config_template_file}" "${wz2100_autorevision_cache_file}" "${CMAKE_CURRENT_SOURCE_DIR}/autorevision_netplay.cmake"
	VERBATIM
)

add_custom_target(autorevision_netcodeversion ALL
	DEPENDS
		"${CMAKE_CURRENT_BINARY_DIR}/netplay_config.h"
)
set_property(TARGET autorevision_netcodeversion PROPERTY FOLDER "_WZBuildProcessTargets")
add_dependencies(autorevision_netcodeversion autorevision) # Ensure ordering and non-concurrency

############################
# netplay library

file(GLOB_RECURSE HEADERS "*.h")

set (SRC
	"byteorder_funcs_wrapper.cpp"
	"client_connection.cpp"
	"connection_provider_registry.cpp"
	"error_categories.cpp"
	"listen_socket.cpp"
	"netjoin_stub.cpp"
	"netlog.cpp"
	"netpermissions.cpp"
	"netplay.cpp"
	"netqueue.cpp"
	"netreplay.cpp"
	"nettypes.cpp"
	"pending_writes_manager.cpp"
	"pending_writes_manager_map.cpp"
	"polling_util.cpp"
	"port_mapping_manager_impl_libplum.cpp"
	"port_mapping_manager_impl_miniupnpc.cpp"
	"port_mapping_manager.cpp"
	"sync_debug.cpp"
	"wz_compression_provider.cpp"
	"wz_connection_provider.cpp"
	"wzfile.cpp"
	"zlib_compression_adapter.cpp"
	"tcp/tcp_address_resolver.cpp"
	"tcp/netsocket.cpp"
	"tcp/sock_error.cpp"
	"tcp/tcp_client_connection.cpp"
	"tcp/tcp_connection_address.cpp"
	"tcp/tcp_connection_poll_group.cpp"
	"tcp/tcp_connection_provider.cpp"
	"tcp/tcp_listen_socket.cpp")

if (ENABLE_GNS_NETWORK_BACKEND)
	list(APPEND SRC
		"gns/gns_client_connection.cpp"
		"gns/gns_connection_address.cpp"
		"gns/gns_connection_poll_group.cpp"
		"gns/gns_connection_provider.cpp"
		"gns/gns_error_categories.cpp"
		"gns/gns_listen_socket.cpp")
endif()

if(MSVC AND CMAKE_VERSION VERSION_GREATER 3.7)
	# Automatic detection of source groups via `source_group(TREE <root>)` syntax
	# has been introduced in CMake 3.8.
	# Please consult https://cmake.org/cmake/help/latest/command/source_group.html for additional info.
	source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Sources" FILES ${SRC})
	source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Headers" FILES ${HEADERS})
endif()

find_package (Threads REQUIRED)
find_package (ZLIB REQUIRED)

# Attempt to find Miniupnpc (minimum supported API version = 9)
# NOTE: This is not available on every platform / distro
find_package(Miniupnpc 9)
if(MINIUPNPC_FOUND)
	set(WZ_USE_IMPORTED_MINIUPNPC ON)
else()
	message(STATUS "Using in-tree Miniupnpc")
	set(WZ_USE_IMPORTED_MINIUPNPC OFF)
	SET(UPNPC_BUILD_STATIC ON CACHE BOOL "miniupnpc - Build static library" FORCE)
	SET(UPNPC_BUILD_SHARED OFF CACHE BOOL "miniupnpc - Build shared library" FORCE)
	SET(UPNPC_BUILD_TESTS OFF CACHE BOOL "miniupnpc - Build tests" FORCE)
	SET(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "miniupnpc - Build samples" FORCE)
	SET(UPNPC_NO_INSTALL TRUE CACHE BOOL "miniupnpc - Disable installation" FORCE)
	add_subdirectory(3rdparty/miniupnp/miniupnpc)
	set_property(TARGET libminiupnpc-static PROPERTY FOLDER "3rdparty")
	if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND (MINGW OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT MSVC)))
		target_compile_options(miniupnpc-private INTERFACE -Wno-macro-redefined)
	endif()
endif()

add_library(netplay STATIC ${HEADERS} ${SRC} "${_netplay_config_output_file}")
add_dependencies(netplay autorevision_netcodeversion)
set_property(TARGET netplay PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(netplay)
target_link_libraries(netplay
	PRIVATE framework re2::re2 nlohmann_json plum-static Threads::Threads ZLIB::ZLIB fmt::fmt
	PUBLIC tl::expected)

if(WZ_USE_IMPORTED_MINIUPNPC)
	target_link_libraries(netplay PRIVATE imported-miniupnpc)
else()
	# Link with in-tree miniupnpc
	target_link_libraries(netplay PRIVATE libminiupnpc-static)
	target_include_directories(netplay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/miniupnp")
endif()

if(MSVC)
	# C4267: 'conversion': conversion from 'type1' to 'type2', possible loss of data // FIXME!!
	target_compile_options(netplay PRIVATE "/wd4267")
endif()

if(ENABLE_GNS_NETWORK_BACKEND)
	target_link_libraries(netplay PUBLIC GameNetworkingSockets::static)
endif()
