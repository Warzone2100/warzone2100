cmake_minimum_required(VERSION 3.19...3.30)

set(sentry_cli_version "2.32.1") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

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
    set(sentry_cli_dl_sha512 "de76de02e389a1c9f7c46d35d89097c4a32ff40d2b423143eda24068ce540e7f9e3d989723cda4928c394cdeda26ebfb2f0e2182124eea1b38c3e1296b03191d")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "45abc55491443e6b228339bbd6bcf74dab07d08093a9a1e249d214a88c3e88aade20198d35e2f89a8614b1100c08420d19bfb3d6011724f069c83af766cf2a0e")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "6ab3a2d6b4e2a8ab2742478eee0bbbee96bbede600155e43650676f3925b6d0813a602498b1a1e9f962fb9d8a922459389259bd1d3a9646d6d21a18abe9ed90c")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "17167bece1e852073aae563bd9f3d482e83ef263566d56c5c37480eb6b58d6ec3e938cae85c0a23b30b2fa26d77319a7c217495722664c2ab00ec47665c7472c")
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
