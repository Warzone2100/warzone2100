cmake_minimum_required(VERSION 3.14)

set(sentry_cli_version "2.15.2") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

# Construct the appropriate URL based on the current platform
set(sentry_cli_dl_url "")
set(sentry_cli_dl_sha512 "")
set(_exe_suffix "")
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-i686.exe")
    set(sentry_cli_dl_sha512 "d32101f0f62c1efc7759455a18da8749db47b3a41cdb0b21c2204beddaf8d0a8726f51af722e5d957d8cf0b8bfaeb0af59bf19b0e9e2464d5af33b9c1ecbd8d6")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "9155dd33c43c03a0d3b0d56f7106d4e1ec197eed2dd7dfec5c1cf0d2d6c0dd5db7097dcd4ab5c59924b9a460a56a8b495ea02adb6fce99097813e042ccfd419b")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "a96bf31d2be0441e68300a037a7845b41e4ad1473dac10a88d6c7408d901fe9c075b973b0c89561e3181a3594efad15b1ff40c10abdbfe01035301966d2adec5")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "919e5b830d14e5a6672e1e9e9ab1fe168aca684f94b706b6bc26a591a4e0a08972fa9808dd07fa734810463d46bf7d34b17a681eac94c77f2f0505080e3186de")
  else()
    message(FATAL_ERROR "Script does not currently support platform: ${CMAKE_HOST_SYSTEM_NAME} and ARCH: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif()
else()
  message(FATAL_ERROR "Script does not currently support platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

include(FetchContent)
FetchContent_Populate(
  sentry-cli
  URL        "${sentry_cli_dl_url}"
  URL_HASH   SHA512=${sentry_cli_dl_sha512}
  DOWNLOAD_NAME "sentry-cli${_exe_suffix}"
  DOWNLOAD_NO_EXTRACT TRUE
  SOURCE_DIR sentry-cli
)

message(STATUS "Downloaded sentry-cli (${sentry_cli_version}) to: \"${CMAKE_CURRENT_BINARY_DIR}/sentry-cli/sentry-cli${_exe_suffix}\"")
