#
# Copyright © 2022 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
# Script Version: 2022-10-12a
#

function(get_pe_file_machine_raw_bytes filename outputvar)

	set(_dos_header_length_bytes 64)

	# read the only bytes from IMAGE_DOS_HEADER we care about
	file(READ "${filename}" _dos_header LIMIT ${_dos_header_length_bytes} HEX)
	
	# magic - "MZ"
	string(SUBSTRING "${_dos_header}" 0 4 _dos_magic_hex)
	if (NOT _dos_magic_hex STREQUAL "4d5a") # "MZ"
		message(WARNING "Missing \"MZ\" start to dos header: ${filename}")
		set(${outputvar} FALSE)
		return()
	endif()
	
	# pe offset
	string(SUBSTRING "${_dos_header}" 120 2 _offset_hex_nib1)
  string(SUBSTRING "${_dos_header}" 122 2 _offset_hex_nib2)
  string(SUBSTRING "${_dos_header}" 124 2 _offset_hex_nib3)
  string(SUBSTRING "${_dos_header}" 126 2 _offset_hex_nib4)
  set(_offset_hex "${_offset_hex_nib4}${_offset_hex_nib3}${_offset_hex_nib2}${_offset_hex_nib1}")
	math(EXPR _pe_offset "0x${_offset_hex}" OUTPUT_FORMAT DECIMAL)

	# read the first 6 bytes of the PE header (DWORD Signature + WORD Machine)
	file(READ "${filename}" _pe_header_start OFFSET ${_pe_offset} LIMIT 6 HEX)
	
	string(SUBSTRING "${_pe_header_start}" 0 4 _pe_sig_hex)
	if (NOT _pe_sig_hex STREQUAL "5045") # "PE"
		message(WARNING "Missing \"PE\" signature to start PE header (got: ${_pe_sig_hex}): ${filename}")
		set(${outputvar} FALSE)
		return()
	endif()
	
	# get the bytes of the machine word (as hex)
	string(SUBSTRING "${_pe_header_start}" 8 4 _machine_bytes_hex)
	
	set(${outputvar} "${_machine_bytes_hex}" PARENT_SCOPE)

endfunction()

function(pe_file_machine_raw_bytes_to_arch machine_bytes_hex outputvar)

	# See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
	if (machine_bytes_hex STREQUAL "4c01")
		set(${outputvar} "i386" PARENT_SCOPE)
	elseif (machine_bytes_hex STREQUAL "6486")
		set(${outputvar} "amd64" PARENT_SCOPE)
	elseif (machine_bytes_hex STREQUAL "64aa")
		set(${outputvar} "arm64" PARENT_SCOPE)
	else()
    message(STATUS "Unknown arch: machine_bytes_hex (raw bytes as hex)")
		set(${outputvar} "unknown" PARENT_SCOPE)
	endif()

endfunction()

# GET_PE_FILE_ARCH( <filename> <outputvar> )
#
# Read and parse the Machine field of the PE file header to get a "pretty" machine type.
#
# Currently supports:
#	- "i386"
# - "amd64"
# - "arm64"
#
# or "unknown"
#
function(get_pe_file_arch filename outputvar)
	
	# get the bytes of the machine word (as hex)
	get_pe_file_machine_raw_bytes("${filename}" _machine_bytes_hex)
	
	pe_file_machine_raw_bytes_to_arch("${_machine_bytes_hex}" ${outputvar})

endfunction()
