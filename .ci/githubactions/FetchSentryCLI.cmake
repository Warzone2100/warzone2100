cmake_minimum_required(VERSION 3.19...3.31)

set(sentry_cli_version "2.58.2") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

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
    set(sentry_cli_dl_sha512 "5f0198bc88745663bcf8bdcba035a94a63c8b90e1b4ac27496364fa9844b0421f75f37bd88e7484521f19992d551d69f2c9a4ae7e9bf020ba48afb094b754a84")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "e1e85fa7139e90502b536e3852cc490c25292711fb08215a5f098b147f6b1bf4ec5568b48e0e00592083109f9a0d27c34255b6aa1666ab3535585bfe5d0cfa93")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "30bd44275e788449c0ba371997e897845fcc6487d775f1cdea0799d888ece694d9ae42f2a949f0469988d10313006d546b53c1d47c2bab03e4dd274eef686898")
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
