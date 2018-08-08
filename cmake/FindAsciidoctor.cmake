# License: MIT License ( https://opensource.org/licenses/MIT )
#
# CMake module to find Asciidoctor
#
# Variables generated:
#
# Asciidoctor_FOUND     true when Asciidoctor_COMMAND is valid
# Asciidoctor_COMMAND   The command to run Asciidoctor
# Asciidoctor_VERSION   The Asciidoctor version that has been found
#

# Try to find an Asciidoctor executable
find_program(ASCIIDOCTOR_EXECUTABLE asciidoctor)
find_program(ASCIIDOCTOR_BAT asciidoctor.bat)

if(ASCIIDOCTOR_EXECUTABLE)
	execute_process(
		COMMAND ${ASCIIDOCTOR_EXECUTABLE} --version
		OUTPUT_VARIABLE _ASCIIDOCTOR_COMMAND_OUTPUT
		ERROR_VARIABLE _ASCIIDOCTOR_COMMAND_ERROR
		RESULT_VARIABLE _ASCIIDOCTOR_COMMAND_RESULT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	# If success, set the ASCIIDOCTOR_COMMAND
	if(_ASCIIDOCTOR_COMMAND_RESULT MATCHES "^0$")
		set(Asciidoctor_COMMAND "${ASCIIDOCTOR_EXECUTABLE}")

	# Else, check if running the ASCIIDOCTOR_BAT succeeds (on Windows)
	elseif((CMAKE_HOST_SYSTEM_NAME MATCHES "Windows") AND ASCIIDOCTOR_BAT)
		execute_process(
			COMMAND ${ASCIIDOCTOR_BAT} --version
			OUTPUT_VARIABLE _ASCIIDOCTOR_COMMAND_OUTPUT
			ERROR_VARIABLE _ASCIIDOCTOR_COMMAND_ERROR
			RESULT_VARIABLE _ASCIIDOCTOR_COMMAND_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		if(_ASCIIDOCTOR_COMMAND_RESULT MATCHES "^0$")
			set(Asciidoctor_COMMAND "${ASCIIDOCTOR_BAT}")
		endif()
	endif()
endif()

# If a command was found, check the version
if(Asciidoctor_COMMAND)
	string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" Asciidoctor_VERSION "${_ASCIIDOCTOR_COMMAND_OUTPUT}")
endif()

unset(ASCIIDOCTOR_EXECUTABLE)
unset(ASCIIDOCTOR_BAT)
unset(_ASCIIDOCTOR_COMMAND_OUTPUT)
unset(_ASCIIDOCTOR_COMMAND_ERROR)
unset(_ASCIIDOCTOR_COMMAND_RESULT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	Asciidoctor
	REQUIRED_VARS Asciidoctor_COMMAND
	VERSION_VAR Asciidoctor_VERSION
)

MARK_AS_ADVANCED(ASCIIDOCTOR_EXECUTABLE ASCIIDOCTOR_BAT)
