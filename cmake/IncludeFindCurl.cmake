cmake_minimum_required(VERSION 3.5)

# This file finds CURL
#
# Attempts to determine its linked / available SSL backend(s):
# - CURL_SUPPORTED_SSL_BACKENDS
#
# And then attempts to determine if its linked SSL backend(s)
# require specific thread-safety / lock callback initialization
# (see: https://curl.haxx.se/libcurl/c/threaded-ssl.html)
#
# - CURL_GNUTLS_REQUIRES_CALLBACKS
# - CURL_OPENSSL_REQUIRES_CALLBACKS
#
# The above two variables are:
# - defined to `YES` if CURL has the backend, and the backend's
#   version is old enough to require callback initialization
# - defined to `NO` if CURL has the backend, and it can be
#   determined that the backend's version is new enough to *NOT*
#   require callback initialization
# - defined to `UNKNOWN` if CURL has the backend, but the backend's
#   version cannot be determined
#

find_package(CURL CONFIG QUIET) # Deliberately quiet, so we can handle the result
if(NOT CURL_FOUND)
	# CONFIG mode failed - fallback to MODULE mode
	# In CMake 3.5 this does not define a target, but it will in 3.12
	# (see https://cmake.org/cmake/help/git-stage/module/FindCURL.html for details).
	# Until then, define the target ourselves if it is missing.
	find_package(CURL MODULE REQUIRED) # Use REQUIRED the second time to fail out
	if (NOT TARGET CURL::libcurl)
		add_library(CURL::libcurl UNKNOWN IMPORTED)
		set_property(TARGET CURL::libcurl
			APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}"
		)
		set_property(TARGET CURL::libcurl
			APPEND PROPERTY IMPORTED_LOCATION "${CURL_LIBRARY}"
		)
	endif()
else()
	# Determine cURL version found
	set(CURL_VERSION_STRING "${CURL_VERSION}")
endif()

##############################################################
# Attempt to determine which SSL backend(s) cURL is linked to

STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CURL_VERSION_STRING "${CURL_VERSION_STRING}")
message(STATUS "CURL_VERSION_STRING=\"${CURL_VERSION_STRING}\"")

unset(CURL_SUPPORTED_SSL_BACKENDS)
unset(CURL_GNUTLS_REQUIRES_CALLBACKS)
unset(CURL_OPENSSL_REQUIRES_CALLBACKS)

find_program(CURL_CONFIG_EXECUTABLE NAMES curl-config)
if(CURL_CONFIG_EXECUTABLE)
	set(_min_curl_version_for_ssl_backends "7.58.0")
	if ((CURL_VERSION_STRING VERSION_EQUAL "${_min_curl_version_for_ssl_backends}") OR (CURL_VERSION_STRING VERSION_GREATER "${_min_curl_version_for_ssl_backends}"))

		execute_process(COMMAND ${CURL_CONFIG_EXECUTABLE} --ssl-backends
						OUTPUT_VARIABLE CURL_CONFIG_SSL_BACKENDS_STRING
						ERROR_QUIET
						OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE "," ";" CURL_SUPPORTED_SSL_BACKENDS "${CURL_CONFIG_SSL_BACKENDS_STRING}")

	else()

		# cURL < 7.58.0 does not support curl-config --ssl-backends
		# Try to parse ssl backend (options) from curl-config --configure
		execute_process(COMMAND ${CURL_CONFIG_EXECUTABLE} --configure
						OUTPUT_VARIABLE CURL_CONFIG_CONFIGURE_STRING
						ERROR_QUIET
						OUTPUT_STRIP_TRAILING_WHITESPACE)
		message(STATUS "CURL_CONFIG_CONFIGURE_STRING=\"${CURL_CONFIG_CONFIGURE_STRING}\"")
		set(CURL_SUPPORTED_SSL_BACKENDS)
		if ((CURL_CONFIG_CONFIGURE_STRING MATCHES "--with-ssl") OR (NOT CURL_CONFIG_CONFIGURE_STRING MATCHES "--without-ssl"))
			# Linked to OpenSSL (or similar)
			list(APPEND CURL_SUPPORTED_SSL_BACKENDS "OpenSSL")
		endif()
		if (CURL_CONFIG_CONFIGURE_STRING MATCHES "--with-gnutls")
			# Linked to GnuTLS
			list(APPEND CURL_SUPPORTED_SSL_BACKENDS "GnuTLS")
		endif()
		if (CURL_CONFIG_CONFIGURE_STRING MATCHES "--with-default-ssl-backend=([^'\" \t\n\r]+)")
			set(_default_ssl_backend "${CMAKE_MATCH_1}")
			message(STATUS "default-ssl-backend=\"${_default_ssl_backend}\"")
			# A default ssl backend was specified - need to parse and check if it's OpenSSL or GnuTLS
			# Filter the CURL_SUPPORTED_SSL_BACKENDS list
			if (_default_ssl_backend MATCHES "gnutls")
				# If GnuTLS is the default, remove OpenSSL from the list of backends processed
				list(REMOVE_ITEM CURL_SUPPORTED_SSL_BACKENDS "OpenSSL")
			elseif (_default_ssl_backend MATCHES "openssl")
				# If OpenSSL is the default, remove GnuTLS from the list of backends processed
				list(REMOVE_ITEM CURL_SUPPORTED_SSL_BACKENDS "GnuTLS")
			else()
				# Anything else the default? Remove both, and ensure the default backend is in the list
				list(REMOVE_ITEM CURL_SUPPORTED_SSL_BACKENDS "GnuTLS")
				list(REMOVE_ITEM CURL_SUPPORTED_SSL_BACKENDS "OpenSSL")
				list(APPEND CURL_SUPPORTED_SSL_BACKENDS "${_default_ssl_backend}")
			endif()
		else()
			# TODO: Handle "implicit default" case?
		endif()
	endif()

	message(STATUS "CURL_SUPPORTED_SSL_BACKENDS=\"${CURL_SUPPORTED_SSL_BACKENDS}\"")

	if ("GnuTLS" IN_LIST CURL_SUPPORTED_SSL_BACKENDS)
		# GnuTLS found
		find_package(GnuTLS QUIET)
		if(GNUTLS_FOUND)
			# Detect GnuTLS version #
			if(NOT DEFINED GNUTLS_VERSION_STRING OR GNUTLS_VERSION_STRING STREQUAL "")
				foreach(_gnutls_include_dir ${GNUTLS_INCLUDE_DIRS})
					# message(STATUS "_gnutls_include_dir: \"${_gnutls_include_dir}\"")
					set(_expected_gnutls_include_file "${_gnutls_include_dir}/gnutls/gnutls.h")
					if(EXISTS "${_expected_gnutls_include_file}")
						# message(STATUS "Searching in file: ${_expected_gnutls_include_file}")
						file(STRINGS "${_expected_gnutls_include_file}" GNUTLS_VERSION_STRING_LINE REGEX "^#define[ \t]+GNUTLS_VERSION[ \t]+\"[.0-9]+\"$")
						if(NOT DEFINED GNUTLS_VERSION_STRING_LINE OR GNUTLS_VERSION_STRING_LINE STREQUAL "")
							# Fall-back to LIBGNUTLS_VERSION, for earlier versions
							file(STRINGS "${_expected_gnutls_include_file}" GNUTLS_VERSION_STRING_LINE REGEX "^#define[ \t]+LIBGNUTLS_VERSION[ \t]+\"[.0-9]+\"$")
						endif()
						string(REGEX REPLACE "^#define[ \t]+[^ \t]*GNUTLS_VERSION[ \t]+\"([.0-9]+)\"$" "\\1" GNUTLS_VERSION_STRING "${GNUTLS_VERSION_STRING_LINE}")
						unset(GNUTLS_VERSION_STRING_LINE)
						break()
					endif()
				endforeach()
			endif()
			message(STATUS "GNUTLS_VERSION_STRING=\"${GNUTLS_VERSION_STRING}\"")
			if (GNUTLS_VERSION_STRING VERSION_LESS "2.11.0")
				# Explicit gcry_control() is required when GnuTLS < 2.11.0
				set(CURL_GNUTLS_REQUIRES_CALLBACKS "YES")
			else()
				# No explicit callback setup is required
				set(CURL_GNUTLS_REQUIRES_CALLBACKS "NO")
			endif()
		else()
			# cURL is linked to GnuTLS, but GnuTLS was not found
			set(CURL_GNUTLS_REQUIRES_CALLBACKS "UNKNOWN")
		endif()
	endif()
	if ("OpenSSL" IN_LIST CURL_SUPPORTED_SSL_BACKENDS)
		# OpenSSL found
		find_package(OpenSSL QUIET)
		if(OPENSSL_FOUND)
			string(REGEX REPLACE "^([0-9]+[.][0-9]+[.][0-9]+).*$" "\\1" OPENSSL_VERSION_NUMBERS "${OPENSSL_VERSION}")
			message(STATUS "OPENSSL_VERSION_NUMBERS=${OPENSSL_VERSION_NUMBERS}")
			if (OPENSSL_VERSION_NUMBERS VERSION_LESS "1.1.0")
				# OpenSSL < 1.1.0 requires thread id and locking callbacks to be initialized
				set(CURL_OPENSSL_REQUIRES_CALLBACKS "YES")
			else()
				# No callbacks are required for OpenSSL >= 1.1.0
				set(CURL_OPENSSL_REQUIRES_CALLBACKS "NO")
			endif()
		else()
			# cURL is linked to OpenSSL, but OpenSSL was not found
			set(CURL_OPENSSL_REQUIRES_CALLBACKS "UNKNOWN")
		endif()
	endif()
else()
	# curl-config was not found; if curl is built with ssl backend(s) OpenSSL or GnuTLS, this may result in thread-safety issues
	unset(CURL_SUPPORTED_SSL_BACKENDS)
endif()
