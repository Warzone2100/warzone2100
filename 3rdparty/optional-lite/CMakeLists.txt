# A very simple version of: https://github.com/martinmoene/optional-lite/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.5)

project(
    optional_lite
#   DESCRIPTION "A C++17-like optional, a nullable object for C++98, C++11 and later in a single-file header-only library"
#   HOMEPAGE_URL "https://github.com/martinmoene/optional-lite"
    LANGUAGES CXX )

# Package information:

set( unit_name       "optional" )
set( package_nspace  "nonstd" )
set( package_name    "${unit_name}-lite" )

#
# Interface, installation and packaging
#

# Interface library:

add_library(
    ${package_name} INTERFACE )

add_library(
    ${package_nspace}::${package_name} ALIAS ${package_name} )

target_include_directories(
    ${package_name}
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
