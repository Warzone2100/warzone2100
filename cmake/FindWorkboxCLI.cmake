# FindWorkboxCLI
# --------------
#
# This module finds a globally-installed workbox-cli.
# It sets the following variables:
#
#  WorkboxCLI_FOUND - True when workbox-cli is found
#  WorkboxCLI_COMMAND - The command used to execute workbox-cli

find_package(NPM REQUIRED)

# Check for workbox-cli (global install)
execute_process(COMMAND
	${CMAKE_COMMAND} -E env NPM_CONFIG_PREFIX=${NPM_GLOBAL_PREFIX_DIR} ${NPM_EXECUTABLE} list -g workbox-cli
	OUTPUT_VARIABLE NPM_LIST_GLOBAL_WORKBOX_CLI_output
	ERROR_VARIABLE NPM_LIST_GLOBAL_WORKBOX_CLI_error
	RESULT_VARIABLE NPM_LIST_GLOBAL_WORKBOX_CLI_result_code
)

if (NPM_LIST_GLOBAL_WORKBOX_CLI_result_code EQUAL 0)
	set(WorkboxCLI_COMMAND "${CMAKE_COMMAND}" -E env "NPM_CONFIG_PREFIX=${NPM_GLOBAL_PREFIX_DIR}" "${NPM_EXECUTABLE}" exec -no -- workbox-cli)
else()
	if(WorkboxCLI_FIND_REQUIRED)
		message(SEND_ERROR "Command \"${NPM_EXECUTABLE} list -g workbox-cli\" failed with output:\n${NPM_LIST_GLOBAL_WORKBOX_CLI_error}")
	else()
		message(STATUS "Command \"${NPM_EXECUTABLE} list -g workbox-cli\" failed with output:\n${NPM_LIST_GLOBAL_WORKBOX_CLI_error}")
	endif()
endif()

unset(NPM_LIST_GLOBAL_WORKBOX_CLI_output)
unset(NPM_LIST_GLOBAL_WORKBOX_CLI_error)
unset(NPM_LIST_GLOBAL_WORKBOX_CLI_result_code)

find_package_handle_standard_args(WorkboxCLI
    REQUIRED_VARS WorkboxCLI_COMMAND
)

mark_as_advanced(NPM_GLOBAL_PREFIX_DIR NPM_EXECUTABLE NPM_VERSION)
