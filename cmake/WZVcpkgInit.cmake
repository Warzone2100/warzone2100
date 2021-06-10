function(WZ_CONVERT_PATH_TO_VCPKG_PATH_VAR _OUTPUTVAR _INPUTPATH)
	if(NOT CMAKE_HOST_WIN32)
		string(REPLACE ":" ";" _INPUTPATH "${_INPUTPATH}")
	endif()
	set(${_OUTPUTVAR} "${_INPUTPATH}" PARENT_SCOPE)
endfunction(WZ_CONVERT_PATH_TO_VCPKG_PATH_VAR)

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()
if(NOT DEFINED VCPKG_OVERLAY_TRIPLETS)
	if(DEFINED ENV{VCPKG_OVERLAY_TRIPLETS})
		WZ_CONVERT_PATH_TO_VCPKG_PATH_VAR(VCPKG_OVERLAY_TRIPLETS "$ENV{VCPKG_OVERLAY_TRIPLETS}")
	endif()
	set(_build_dir_overlay_triplets "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_overlay_triplets")
	if(EXISTS "${_build_dir_overlay_triplets}" AND IS_DIRECTORY "${_build_dir_overlay_triplets}")
		list(APPEND VCPKG_OVERLAY_TRIPLETS "${_build_dir_overlay_triplets}")
	endif()
	unset(_build_dir_overlay_triplets)
	if(DEFINED VCPKG_OVERLAY_TRIPLETS)
		set(VCPKG_OVERLAY_TRIPLETS "${VCPKG_OVERLAY_TRIPLETS}" CACHE STRING "")
	endif()
endif()
