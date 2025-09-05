cmake_minimum_required(VERSION 3.19...3.31)

set(sentry_cli_version "2.53.0") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

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
    message(FATAL_ERROR "Script does not currently support platform: ${CMAKE_HOST_SYSTEM_NAME} and ARCH: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "8903da75117f5eab10c08b9b925e6f6442b06685974feb5960e37a91221860a57ca481c50c0bc4eb4e3c5b36d6712a238e209f514487cdd96d7213fa6d893584")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "dc9374a26e94f9cb76193dbbe3d29c1baa3e0d2969d1ad2ed09975bdcb7aeb3dc795d8dca745536e8919c3af3b6fdc042649a58174825add0790b34e2852e974")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "f9a4f4be6bdb698ca6dd1ca4de7ac95dc7e5360726a36e9c63558692662420e91d86200830d831f6426715087310890e6172c822f5be5e3bc575c2ec0f6d3c2e")
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
