cmake_minimum_required(VERSION 3.5...3.24)

# Generate the multiplayer compatibility version in netplay_config.h.
#
# Required input defines:
# - TEMPLATE_FILE: the full filename + path for the input netplay_config.h.in template file
# - OUTPUT_FILE: the full filename + path for the output netplay_config.h file
#

if(NOT DEFINED TEMPLATE_FILE OR "${TEMPLATE_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: TEMPLATE_FILE" )
endif()
if(NOT DEFINED OUTPUT_FILE OR "${OUTPUT_FILE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: OUTPUT_FILE" )
endif()

#################################

# This is deliberately independent of release tags, branches, and commit counts.
# 0x5000 starts a namespace above all values produced by the old metadata-based
# scheme. See doc/MultiplayerCompatibility.md before changing either value.
set(NETCODE_VERSION_MAJOR 0x5000)
set(NETCODE_VERSION_MINOR 0)

##################################
# Debug output

execute_process(COMMAND ${CMAKE_COMMAND} -E echo "++NETCODE compatibility version: ${NETCODE_VERSION_MAJOR}.${NETCODE_VERSION_MINOR}")

##################################
# Output configured file based on the template

if(NOT EXISTS "${TEMPLATE_FILE}")
	message( FATAL_ERROR "Input TEMPLATE_FILE does not exist: \"${TEMPLATE_FILE}\"" )
endif()
configure_file("${TEMPLATE_FILE}" "${OUTPUT_FILE}" @ONLY)
