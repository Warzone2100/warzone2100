cmake_minimum_required (VERSION 3.16...3.31)

# MikkTSpace - the reference tangent-space implementation
#
# Kept as an unmodified drop-in copy: its whole value is that every tool which
# bakes normal maps (Blender, Substance, xNormal, Marmoset) uses this exact
# code, so tangent frames agree across the pipeline. (Do not patch it locally.)
add_library(mikktspace STATIC "mikktspace.h" "mikktspace.c")
set_property(TARGET mikktspace PROPERTY FOLDER "3rdparty")
target_include_directories(mikktspace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT MSVC)
	# upstream is warning-clean under its own settings but not under ours
	target_compile_options(mikktspace PRIVATE -w)
endif()
