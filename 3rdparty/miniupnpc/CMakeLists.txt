cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.c")

add_library(miniupnpc STATIC ${HEADERS} ${SRC})
target_include_directories(miniupnpc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_definitions(miniupnpc PUBLIC "MINIUPNP_STATICLIB")

if (MSVC)
	if (NOT (MSVC_VERSION VERSION_LESS 1400))
		target_compile_definitions(miniupnpc PUBLIC
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
endif()
