cmake_minimum_required(VERSION 3.5...3.24)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Remove the files that workbox-cli generates as output from the current/working directory
# - service-worker.js
# - service-worker.js.map
# - workbox-*.js
# - workbox-*.js.map

file(GLOB WORKBOX_GENERATED_FILES LIST_DIRECTORIES false
	"${CMAKE_CURRENT_SOURCE_DIR}/service-worker.js"
	"${CMAKE_CURRENT_SOURCE_DIR}/service-worker.js.map"
	"${CMAKE_CURRENT_SOURCE_DIR}/workbox-*.js"
	"${CMAKE_CURRENT_SOURCE_DIR}/workbox-*.js.map"
)

foreach(_file IN LISTS WORKBOX_GENERATED_FILES)
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "Removing old generated file: ${_file}")
	file(REMOVE "${_file}")
endforeach()
