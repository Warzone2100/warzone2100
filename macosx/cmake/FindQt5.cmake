# FindQt5 for macOS builds
# Using minimal bundle of required frameworks for WZ

cmake_minimum_required(VERSION 3.5)

get_filename_component(_qt5_framework_prefix "${CMAKE_BINARY_DIR}/macosx/external/QT" ABSOLUTE)

set(_download_script_path "${CMAKE_CURRENT_LIST_DIR}/../configs/fetchscripts/SetupPrebuiltComponents-QT.sh")
if(NOT EXISTS "${_qt5_framework_prefix}")
	# Download the Qt5 minimal bundle for macOS to the build dir
	if(NOT EXISTS "${_download_script_path}")
		message( FATAL_ERROR "Missing required download script: \"${_download_script_path}\"" )
	endif()
	message( STATUS "Downloading missing Qt5 minimal bundle to: \"${_qt5_framework_prefix}\"" )
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E make_directory macosx
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
	execute_process(
		COMMAND bash ${_download_script_path}
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/macosx"
	)
endif()

if(NOT IS_DIRECTORY "${_qt5_framework_prefix}")
	message( FATAL_ERROR "Expected Qt5 minimal bundle path is not a folder: ${_qt5_framework_prefix}" )
endif()

set(_qt5_failed_to_find_components)

foreach(component ${Qt5_FIND_COMPONENTS})

	set(_qt5_component_framework_path "${_qt5_framework_prefix}/Qt${component}.framework")

	if (NOT TARGET Qt5::${component})

		# Determine if Qt${component}.framework exists
		if(EXISTS "${_qt5_component_framework_path}")

			add_library(Qt5::${component} INTERFACE IMPORTED)

			set_property(TARGET Qt5::${component} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_qt5_component_framework_path}")

			# Special-case handling of required plugins
			if     (component STREQUAL "Core")

			elseif (component STREQUAL "Gui")
				# Add QCocoaIntegrationPlugin
				add_library(Qt5::QCocoaIntegrationPlugin MODULE IMPORTED)
				set_property(TARGET Qt5::QCocoaIntegrationPlugin APPEND PROPERTY
					IMPORTED_LOCATION "${_qt5_framework_prefix}/plugins/platforms/libqcocoa.dylib"
				)
			elseif (component STREQUAL "Widgets")

			elseif (component STREQUAL "PrintSupport")
				# Add QCocoaPrinterSupportPlugin
				add_library(Qt5::QCocoaPrinterSupportPlugin MODULE IMPORTED)
				set_property(TARGET Qt5::QCocoaPrinterSupportPlugin APPEND PROPERTY
					IMPORTED_LOCATION "${_qt5_framework_prefix}/printsupport/libcocoaprintersupport.dylib"
				)
			elseif (component STREQUAL "Script")

			endif()

		else()

			# Specified a Qt5 component that does not exist in the current minimal bundle
			# A new minimal macOS bundle will have to be generated for WZ, containing this additional component (and any dependencies)

			list(APPEND _qt5_failed_to_find_components "${component}")

		endif()

	endif()


endforeach()

set(_MOC_BIN "${_qt5_framework_prefix}/usr/bin/moc")
if (EXISTS "${_MOC_BIN}")

	# Define custom qt5_wrap_cpp implementation
	# (NOTE: This custom implementation expects to be called with an output variable
	# and a list of files to moc. No other options are supported.)

	function(qt5_wrap_cpp output_moc_files)
		set(_options)
		set(_oneValueArgs)
		set(_multiValueArgs)

		CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

		set(source_files ${_parsedArguments_UNPARSED_ARGUMENTS})

		foreach(file_to_moc ${source_files})
			get_filename_component(file_to_moc ${file_to_moc} ABSOLUTE)

			# take the input file name (without extension), and append ".moc.cpp"
			get_filename_component(output_filename ${file_to_moc} NAME_WE)
			set(output_filename "${CMAKE_CURRENT_BINARY_DIR}/${output_filename}.moc.cpp")

			add_custom_command(
				OUTPUT "${output_filename}"
				COMMAND ${_MOC_BIN} -o ${output_filename} ${file_to_moc}
				DEPENDS "${file_to_moc}"
				VERBATIM
			)

			list(APPEND ${output_moc_files} ${output_filename})
		endforeach()

		set(${output_moc_files} "${${output_moc_files}}" PARENT_SCOPE)
	endfunction()

else()

	# Failed to find Qt5 moc in minimal bundle
	list(APPEND _qt5_failed_to_find_components "[/usr/bin/moc]")

endif()


if (_qt5_failed_to_find_components)
	message( FATAL_ERROR "Failed to find Qt5 components in minimal macOS Qt5 bundle: ${_qt5_failed_to_find_components}" )
endif()
