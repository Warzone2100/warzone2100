cmake_minimum_required(VERSION 3.16...3.31)

if(NOT ENABLE_NLS)
	message(STATUS "Skipping translations generation / native language support")
	return()
endif()

find_package(Gettext 0.18)
if(NOT GETTEXT_FOUND)
	message( WARNING "Gettext is not found. Skipping language file generation. (Only the default language, English, will be available.)" )
	return()
endif()
MARK_AS_ADVANCED(GETTEXT_MSGFMT_EXECUTABLE GETTEXT_MSGMERGE_EXECUTABLE)

find_program(XGETTEXT_CMD xgettext)
MARK_AS_ADVANCED(XGETTEXT_CMD)

if(NOT XGETTEXT_CMD)
	MESSAGE( WARNING "xgettext not found. Unable to generate Language translations without Gettext." )
	return()
endif()

# GETTEXT_CREATE_TRANSLATIONS_WZ(potFile [ALL]
#								 [INSTALL_DESTINATION <destdir>]
#								 [INSTALL_COMPONENT component]
#								 [MSGMERGE_OPTIONS <mergeoptions>]
#								 [TARGET_FOLDER <folder>]
#								 POFILES file1 ...  fileN)
#
# Starting point: https://github.com/Kitware/CMake/blob/fb3a608f1ab4a8fda04d5c2fb5ad99e396ebd1e6/Modules/FindGettext.cmake
# Distributed under the OSI-approved BSD 3-Clause License. See https://cmake.org/licensing for details.
#
# Customized for WZ to:
#	- Specify options to msgmerge
#	- Specify an install destination
#	- Specify an install component
#	- Specify a target folder (for any created targets)
#	- Specify a list of po files as POFILES
#	- Specify a distinct output file location for updated .po files in the build directory
#	  (instead of updating in-place, which modifies the source folder)
#	- Use the updated .po files in the build directory for the msgfmt command
#
function(GETTEXT_CREATE_TRANSLATIONS_WZ _potFile)
   set(_options ALL)
   set(_oneValueArgs INSTALL_DESTINATION INSTALL_COMPONENT TARGET_FOLDER)
   set(_multiValueArgs MSGMERGE_OPTIONS POFILES)

   CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

   set(_gmoFiles)
   get_filename_component(_potName ${_potFile} NAME)
   string(REGEX REPLACE "^(.+)(\\.[^.]+)$" "\\1" _potBasename ${_potName})
   get_filename_component(_absPotFile ${_potFile} ABSOLUTE)
   set(_msgmerge_options)
   if (DEFINED _parsedArguments_MSGMERGE_OPTIONS)
      set(_msgmerge_options ${_parsedArguments_MSGMERGE_OPTIONS})
   endif()

   foreach (_currentPoFile ${_parsedArguments_POFILES})
      get_filename_component(_currentPoFileName "${_currentPoFile}" NAME)
      get_filename_component(_absFile "${_currentPoFile}" ABSOLUTE)
      get_filename_component(_abs_PATH "${_absFile}" PATH)
      get_filename_component(_lang "${_absFile}" NAME_WE)

      file(RELATIVE_PATH _po_file_relative_path "${CMAKE_CURRENT_SOURCE_DIR}" "${_absFile}")
      get_filename_component(_po_file_subdir_path "${_po_file_relative_path}" DIRECTORY)

      if (_po_file_relative_path AND NOT _po_file_relative_path STREQUAL "")
         set(_updatedPoFileDirectory "${CMAKE_CURRENT_BINARY_DIR}/${_po_file_subdir_path}")
      else()
         set(_updatedPoFileDirectory "${CMAKE_CURRENT_BINARY_DIR}")
      endif()
      file(MAKE_DIRECTORY "${_updatedPoFileDirectory}")
      set(_updatedPoFile "${_updatedPoFileDirectory}/${_potBasename}_${_currentPoFileName}")
      set(_gmoDirectory "${CMAKE_CURRENT_BINARY_DIR}/locale/${_lang}/LC_MESSAGES")
      set(_gmoFile "${_gmoDirectory}/${_potBasename}.mo")

      add_custom_command(
         OUTPUT "${_updatedPoFile}"
         COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} ${_msgmerge_options} --output-file=${_updatedPoFile} ${_absFile} ${_absPotFile}
         DEPENDS "${_absPotFile}" "${_absFile}"
         VERBATIM
      )

      add_custom_command(
         OUTPUT "${_gmoFile}"
         COMMAND ${CMAKE_COMMAND} -E make_directory ${_gmoDirectory}
         COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -c --statistics -o ${_gmoFile} ${_updatedPoFile}
         DEPENDS "${_absPotFile}" "${_updatedPoFile}"
         VERBATIM
      )

      if(_parsedArguments_INSTALL_DESTINATION)
	     if (DEFINED _parsedArguments_INSTALL_COMPONENT)
            install(FILES "${_gmoFile}" COMPONENT "${_parsedArguments_INSTALL_COMPONENT}" DESTINATION "${_parsedArguments_INSTALL_DESTINATION}/${_lang}/LC_MESSAGES" RENAME ${_potBasename}.mo)
         else()
            install(FILES "${_gmoFile}" DESTINATION "${_parsedArguments_INSTALL_DESTINATION}/${_lang}/LC_MESSAGES" RENAME ${_potBasename}.mo)
         endif()
	  endif()
      list(APPEND _gmoFiles "${_gmoFile}")

   endforeach ()

   if(NOT TARGET translations)
      add_custom_target(translations)
      if (DEFINED _parsedArguments_TARGET_FOLDER)
         set_property(TARGET translations PROPERTY FOLDER "${_parsedArguments_TARGET_FOLDER}")
      endif()
   endif()

   _GETTEXT_GET_UNIQUE_TARGET_NAME(translations uniqueTargetName)

   if(_parsedArguments_ALL)
      add_custom_target(${uniqueTargetName} ALL DEPENDS ${_gmoFiles})
   else()
      add_custom_target(${uniqueTargetName} DEPENDS ${_gmoFiles})
   endif()

   if (DEFINED _parsedArguments_TARGET_FOLDER)
      set_property(TARGET ${uniqueTargetName} PROPERTY FOLDER "${_parsedArguments_TARGET_FOLDER}")
   endif()

   add_dependencies(translations ${uniqueTargetName})

endfunction()

#############################################################
function(BUILD_WZ_TRANSLATIONS textDomain BASE_PO_DIR POTFILES_IN_PATH)

	# Must be set to ${textDomain}.pot, as the filename (without its extension) is used later
	# to form the install .gmo file name (${textDomain}.gmo) in each locale directory

	file(RELATIVE_PATH _po_file_relative_path "${CMAKE_CURRENT_SOURCE_DIR}" "${BASE_PO_DIR}")
	if (_po_file_relative_path AND NOT _po_file_relative_path STREQUAL "")
		set(_potFileDir "${CMAKE_CURRENT_BINARY_DIR}/${_po_file_relative_path}")
	else()
		set(_potFileDir "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	file(MAKE_DIRECTORY "${_potFileDir}")
	set(_potFile "${_potFileDir}/${textDomain}.pot")

	# Read in `POTFILES.in` to build a list of dependency paths in the same order.
	file(STRINGS "${POTFILES_IN_PATH}" potfiles
		REGEX "^[^#].*"
	)
	if (NOT potfiles)
		message(STATUS "${POTFILES_IN_PATH}: No files specified?")
		return()
	endif()
	foreach(potfile ${potfiles})
		if (potfile AND NOT potfile STREQUAL "")
			list(APPEND source_translatable_depends
				"../${potfile}")
		endif()
	endforeach()
	if (NOT source_translatable_depends)
		message(STATUS "${POTFILES_IN_PATH}: No files specified? [2]")
		return()
	endif()

	# Generate the ${textDomain}.pot file in the build directory
	ADD_CUSTOM_COMMAND(
		OUTPUT ${_potFile}
		COMMAND ${CMAKE_COMMAND} -DPOTFILES_IN=${POTFILES_IN_PATH} -DOUTPUT_FILE=${_potFile} -DPACKAGE_NAME=${textDomain} -DXGETTEXT_CMD=${XGETTEXT_CMD} -P ${CMAKE_CURRENT_SOURCE_DIR}/WZ_build_po_template.cmake
		DEPENDS ${POTFILES_IN_PATH} ${source_translatable_depends} "${CMAKE_CURRENT_SOURCE_DIR}/WZ_build_po_template.cmake"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Extract translatable messages to ${_potFile}"
		VERBATIM
	)

	set(_po_install_config)
	if(NOT CMAKE_SYSTEM_NAME MATCHES "Emscripten")
		set(_po_install_config
		INSTALL_DESTINATION "${WZ_LOCALEDIR}"
		INSTALL_COMPONENT Languages)
	endif()

	# Build the *.gmo files from the *.po files (using the .pot file)
	include(GNUInstallDirs)
	file (GLOB POFILES "${BASE_PO_DIR}/*.po")
	if (POFILES)
		GETTEXT_CREATE_TRANSLATIONS_WZ ("${_potFile}" ALL
			${_po_install_config}
			MSGMERGE_OPTIONS --quiet --no-wrap --width=1
			TARGET_FOLDER "po"
			POFILES ${POFILES})
	else()
		message(STATUS "${BASE_PO_DIR}/*.po: No .po files?")
	endif()

endfunction()

###################################
###################################

# On CMake configure, clear the build dir "locale" folder (to ensure re-generation)
set(wz2100_translations_LOCALE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/locale")
file(REMOVE_RECURSE "${wz2100_translations_LOCALE_FOLDER}/")

BUILD_WZ_TRANSLATIONS("warzone2100${WZ_OUTPUT_NAME_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in")

BUILD_WZ_TRANSLATIONS("warzone2100${WZ_OUTPUT_NAME_SUFFIX}_guide" "${CMAKE_CURRENT_SOURCE_DIR}/guide" "${CMAKE_CURRENT_SOURCE_DIR}/guide/POTFILES.in")

set(wz2100_translations_LOCALE_FOLDER "${wz2100_translations_LOCALE_FOLDER}" PARENT_SCOPE)
