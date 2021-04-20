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

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

find_package (Threads REQUIRED)
find_package (ZLIB REQUIRED)

add_library(netplay STATIC ${HEADERS} ${SRC} "${_netplay_config_output_file}")
add_dependencies(netplay autorevision_netcodeversion)
set_property(TARGET netplay PROPERTY FOLDER "lib")
if(WZ_TARGET_ADDITIONAL_PROPERTIES)
	SET_TARGET_PROPERTIES(netplay PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
endif()
target_link_libraries(netplay PRIVATE framework re2::re2)
if(WZ_USE_IMPORTED_MINIUPNPC)
	target_link_libraries(netplay PRIVATE imported-miniupnpc)
else()
	# Link with in-tree miniupnpc
	target_link_libraries(netplay PRIVATE libminiupnpc-static)
	target_include_directories(netplay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/miniupnp")
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	target_link_libraries(netplay PRIVATE pthread)
endif()
target_link_libraries(netplay PRIVATE Threads::Threads ZLIB::ZLIB)
if(MSVC)
	# C4267: 'conversion': conversion from 'type1' to 'type2', possible loss of data // FIXME!!
	target_compile_options(netplay PRIVATE "/wd4267")
endif()
