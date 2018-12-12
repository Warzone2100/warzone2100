cmake_minimum_required(VERSION 3.5)

function(DisallowInSourceBuilds)
	get_filename_component(_real_source_dir "${CMAKE_SOURCE_DIR}" REALPATH)
	get_filename_component(_real_binary_dir "${CMAKE_BINARY_DIR}" REALPATH)
	if(_real_source_dir STREQUAL _real_binary_dir)
		message(FATAL_ERROR "In-source builds are not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt from the source directory.")
	endif()
endfunction()

DisallowInSourceBuilds()
