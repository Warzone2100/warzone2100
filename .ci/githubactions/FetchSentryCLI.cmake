cmake_minimum_required(VERSION 3.19...3.30)

set(sentry_cli_version "2.42.4") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

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
    set(sentry_cli_dl_sha512 "3ca02be7c6ddfe624a694b28f82034bbc0aed204416f09b48401af0cea7b76bc7ca58612ec88963393f56b03bd1695c0094317ddea1da1ece1c17b126b0fef0d")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "05b862dd013bdcc50c277e24081490992206234ca651ce4ee67368fcf48f20be3818897e291105169c893650370ebf107e7f74e929335de18efb7329f1be3c58")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "f346313189674c19897f22ca381f93545968cb002f10d6d88c4693f2f1f8133ebdcf86b82a83924c6086086009095a81ef47bb89fb309f9317295813f1f0f01f")
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
