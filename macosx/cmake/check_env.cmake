cmake_minimum_required(VERSION 3.5)

# Verify an environment variable
#
# Required input defines:
# - NAME: the environment variable name
# - EXPECTED_VALUE: the environment variable expected value
#

if(NOT DEFINED NAME OR "${NAME}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: NAME" )
endif()

if(NOT DEFINED EXPECTED_VALUE OR "${EXPECTED_VALUE}" STREQUAL "")
	message( FATAL_ERROR "Missing required input define: EXPECTED_VALUE" )
endif()

#################################

if(DEFINED ENV{${NAME}})
	if("$ENV{${NAME}}" STREQUAL "${EXPECTED_VALUE}")
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "info: \$ENV{${NAME}} == \"${EXPECTED_VALUE}\"")
	else()
		execute_process(COMMAND ${CMAKE_COMMAND} -E echo "error: The environment variable \"${NAME}\" != \"${EXPECTED_VALUE}\" (actual value: \"$ENV{${NAME}}\")")
		message(FATAL_ERROR "The environment variable \"${NAME}\" != \"${EXPECTED_VALUE}\" (actual value: \"$ENV{${NAME}}\")")
	endif()
else()
	execute_process(COMMAND ${CMAKE_COMMAND} -E echo "error: The environment variable \"${NAME}\" is not set")
	message(FATAL_ERROR "The environment variable \"${NAME}\" is not set")
endif()

