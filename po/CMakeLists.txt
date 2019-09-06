cmake_minimum_required(VERSION 3.5)

find_package(Gettext 0.18)
if(NOT GETTEXT_FOUND)
	message( WARNING "Gettext is not found. Skipping language file generation. (Only the default language, English, will be available.)" )
	if (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
		# Because of a Windows-related bugfix in FindGettext.cmake in CMake 3.10,
		# CMake 3.10+ may be required (if compiling *on* Windows) to properly
		# detect Gettext.
		#
		# However, Visual Studio 2017 currently bundles CMake < 3.10, so requiring
		# CMake 3.10+ at the top of this file leads to an inability to compile via
		# Visual Studio itself.
		#
		# Instead of using cmake_minimum_required(VERSION 3.10), detect the
		# CMake version and emit a warning if it's below 3.10.
		#
		if(${CMAKE_VERSION} VERSION_LESS "3.10.0")
			message ( WARNING "CMake is < 3.10. This *may* be the cause of not finding Gettext on Windows (if it's installed)." )
		endif()
	endif()
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
      set(_updatedPoFile "${CMAKE_CURRENT_BINARY_DIR}/${_currentPoFileName}")
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

# Must be set to warzone2100.pot, as the filename (without its extension) is used later
# to form the install .gmo file name (warzone2100.gmo) in each locale directory
set(_potFile "${CMAKE_CURRENT_BINARY_DIR}/warzone2100.pot")

# Read in `POTFILES.in` to build a list of dependency paths in the same order.
set(_potfiles_in_path "${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in")
file(STRINGS "${_potfiles_in_path}" potfiles
	REGEX "^[^#].*"
)
foreach(potfile ${potfiles})
	list(APPEND source_translatable_depends
		"../${potfile}")
endforeach()

# Generate the warzone2100.pot file in the build directory
ADD_CUSTOM_COMMAND(
	OUTPUT ${_potFile}
	COMMAND ${CMAKE_COMMAND} -DPOTFILES_IN=${_potfiles_in_path} -DOUTPUT_FILE=${_potFile} -DXGETTEXT_CMD=${XGETTEXT_CMD} -P ${CMAKE_CURRENT_SOURCE_DIR}/WZ_build_po_template.cmake
	DEPENDS ${_potfiles_in_path} ${source_translatable_depends} "${CMAKE_CURRENT_SOURCE_DIR}/WZ_build_po_template.cmake"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Extract translatable messages to ${_potFile}"
	VERBATIM
)

ADD_CUSTOM_TARGET(pot_file ALL
	DEPENDS ${_potFile})
set_property(TARGET pot_file PROPERTY FOLDER "po")

set(wz2100_translations_LOCALE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/locale")

# On CMake configure, clear the build dir "locale" folder (to ensure re-generation)
file(REMOVE_RECURSE "${wz2100_translations_LOCALE_FOLDER}/")

# Build the *.gmo files from the *.po files (using the .pot file)
include(GNUInstallDirs)
file (GLOB POFILES *.po)
GETTEXT_CREATE_TRANSLATIONS_WZ ("${_potFile}" ALL
	INSTALL_DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
	INSTALL_COMPONENT Languages
	MSGMERGE_OPTIONS --quiet --no-wrap --width=1
	TARGET_FOLDER "po"
	POFILES ${POFILES})

set(wz2100_translations_LOCALE_FOLDER "${wz2100_translations_LOCALE_FOLDER}" PARENT_SCOPE)
