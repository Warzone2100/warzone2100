cmake_minimum_required(VERSION 3.14)

set(sentry_cli_version "2.7.0") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

# Construct the appropriate URL based on the current platform
set(sentry_cli_dl_url "")
set(sentry_cli_dl_sha512 "")
set(_exe_suffix "")
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-i686.exe")
    set(sentry_cli_dl_sha512 "1f7412789091a8d7fd5f7587a8ba508ae07a92b580c4517d3d81c3c5c7293f1d4b1d0acba5a30236151920aa5405131336f5b105bfd310fb7219668c1b3bd73d")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "902f8071df84faddd380cba9d127821b82426b5ad0997eeb01e7678e9f78aad0d85526cc2aaa19f0b2f4bdae35f0a7191fcc5937bae6fdd5fb96113a07bcd0be")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "1f7412789091a8d7fd5f7587a8ba508ae07a92b580c4517d3d81c3c5c7293f1d4b1d0acba5a30236151920aa5405131336f5b105bfd310fb7219668c1b3bd73d")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86_64$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Linux-x86_64")
    set(sentry_cli_dl_sha512 "03d8bc6e3f84204cf78c1909b82e5bcab4c143e1cca093cdf2b458ab300d8340bef4d5e352d618ed5cef10c3c174e8ed38b86b8c832b5964253c313c7a5cce4a")
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
