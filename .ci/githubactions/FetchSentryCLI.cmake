cmake_minimum_required(VERSION 3.19...3.30)

set(sentry_cli_version "2.38.0") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

# Manually query the CMAKE_HOST_SYSTEM_PROCESSOR
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/25151
cmake_host_system_information(RESULT CMAKE_HOST_SYSTEM_PROCESSOR QUERY OS_PLATFORM)
message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR=${CMAKE_HOST_SYSTEM_PROCESSOR}")

# Construct the appropriate URL based on the current platform
set(sentry_cli_dl_url "")
set(sentry_cli_dl_sha512 "")
set(_exe_suffix "")
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-i686.exe")
    set(sentry_cli_dl_sha512 "60d004fc8df9003978596dbc45e5dba8350d47ea8535a0c60875aa937e6159911b345c817111932ae68ec71106e2c6712f8e52c3bf41086ad08db379796b575c")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "427c3e1b7495a9b6c1d3d251281a37c13e71260ef05c52837b31c0c8348910047f3403855d014bf1701f18787849e2403d7ddc869b9d1b8d4985fccf0e530306")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "42c2dbbf694dcce647ccafa15fa482e0736fc50c0c50ffaf7abe02457b106b7f1f9a5b5114b3a2853aa56c5b8e957a84dc9b952c2f1f84c94054280544e7ac9f")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "71c52338f35e7afe1b452abfbfba8b070d7fc770ce7fa3016962aed8eae29b3f6c09c1758574a7f6fe63c9e18aaca465793c874716459e8d35a3dbbe01e44abe")
  else()
    message(FATAL_ERROR "Script does not currently support platform: ${CMAKE_HOST_SYSTEM_NAME} and ARCH: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif()
else()
  message(FATAL_ERROR "Script does not currently support platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(_output_fullpath "${CMAKE_CURRENT_BINARY_DIR}/sentry-cli/sentry-cli${_exe_suffix}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sentry-cli")
file(DOWNLOAD "${sentry_cli_dl_url}" "${_output_fullpath}" SHOW_PROGRESS TLS_VERIFY ON EXPECTED_HASH SHA512=${sentry_cli_dl_sha512})
file(CHMOD "${_output_fullpath}" FILE_PERMISSIONS
	OWNER_EXECUTE OWNER_WRITE OWNER_READ
	GROUP_EXECUTE GROUP_READ
	WORLD_EXECUTE WORLD_READ
)

message(STATUS "Downloaded sentry-cli (${sentry_cli_version}) to: \"${_output_fullpath}\"")
