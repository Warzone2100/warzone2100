# SOURCE denotes the file or directory to copy.
# DEST_DIR denotes the directory for copy to.
if(IS_DIRECTORY "${SOURCE}")
	set(SOURCE "${SOURCE}/")
	set(_dest_subdir "${SOURCE}")
else()
	get_filename_component(_dest_subdir "${SOURCE}" DIRECTORY)
endif()
file(COPY "${SOURCE}" DESTINATION "${DEST_DIR}/${_dest_subdir}"
	PATTERN ".git*" EXCLUDE
)
