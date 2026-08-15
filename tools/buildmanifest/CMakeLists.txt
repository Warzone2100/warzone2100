cmake_minimum_required(VERSION 3.16...3.31)
project(wzbuildmanifest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(WZ_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${WZ_REPO_ROOT}/cmake")
find_package(Sodium REQUIRED)

add_executable(wzbuildmanifest
	src/main.cpp
	"${WZ_REPO_ROOT}/src/buildprovenance/wzbuildcert.cpp"
)
target_include_directories(wzbuildmanifest PRIVATE
	"${WZ_REPO_ROOT}/src/buildprovenance"
	"${WZ_REPO_ROOT}/3rdparty/json/include"
	"${WZ_REPO_ROOT}/3rdparty/optional-lite/include"
)
target_link_libraries(wzbuildmanifest PRIVATE unofficial-sodium::sodium)
