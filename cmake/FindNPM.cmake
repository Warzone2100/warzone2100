# FindNPM
# --------
#
# This module finds an installed npm.
# It sets the following variables:
#
#  NPM_FOUND - True when NPM is found
#  NPM_GLOBAL_PREFIX_DIR - The global prefix directory
#  NPM_EXECUTABLE - The path to the npm executable
#  NPM_VERSION - The version number of the npm executable

find_program(NPM_EXECUTABLE NAMES npm HINTS /usr)

if (NPM_EXECUTABLE)

	# Get the global npm prefix
	execute_process(COMMAND ${NPM_EXECUTABLE} prefix -g
		OUTPUT_VARIABLE NPM_GLOBAL_PREFIX_DIR
		ERROR_VARIABLE NPM_prefix_g_error
		RESULT_VARIABLE NPM_prefix_g_result_code
	)
	# Remove spaces and newlines
	string (STRIP ${NPM_GLOBAL_PREFIX_DIR} NPM_GLOBAL_PREFIX_DIR)
	if (NPM_prefix_g_result_code)
		if(NPM_FIND_REQUIRED)
			message(SEND_ERROR "Command \"${NPM_EXECUTABLE} prefix -g\" failed with output:\n${NPM_prefix_g_error}")
		else()
			message(STATUS "Command \"${NPM_EXECUTABLE} prefix -g\" failed with output:\n${NPM_prefix_g_error}")
		endif()
	endif()
	unset(NPM_prefix_g_error)
	unset(NPM_prefix_g_result_code)

	# Get the VERSION
	execute_process(COMMAND ${NPM_EXECUTABLE} -v
		OUTPUT_VARIABLE NPM_VERSION
		ERROR_VARIABLE NPM_version_error
		RESULT_VARIABLE NPM_version_result_code
	)
	if(NPM_version_result_code)
		if(NPM_FIND_REQUIRED)
			message(SEND_ERROR "Command \"${NPM_EXECUTABLE} -v\" failed with output:\n${NPM_version_error}")
		else()
			message(STATUS "Command \"${NPM_EXECUTABLE} -v\" failed with output:\n${NPM_version_error}")
		endif()
	endif()
	unset(NPM_version_error)
	unset(NPM_version_result_code)

	# Remove spaces and newlines
	string (STRIP ${NPM_VERSION} NPM_VERSION)
else()
	if (NPM_FIND_REQUIRED)
		message(SEND_ERROR "Failed to find npm executable")
	endif()
endif()

find_package_handle_standard_args(NPM
	REQUIRED_VARS NPM_EXECUTABLE NPM_GLOBAL_PREFIX_DIR
	VERSION_VAR NPM_VERSION
)

mark_as_advanced(NPM_GLOBAL_PREFIX_DIR NPM_EXECUTABLE NPM_VERSION)
