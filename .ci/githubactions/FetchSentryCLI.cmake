cmake_minimum_required(VERSION 3.14)

set(sentry_cli_version "1.68.0") # Note: When updating, must also update all of the sentry_cli_dl_sha512 below!

# Construct the appropriate URL based on the current platform
set(sentry_cli_dl_url "")
set(sentry_cli_dl_sha512 "")
set(_exe_suffix "")
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^x86$")
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-i686.exe")
    set(sentry_cli_dl_sha512 "b4357fe65930642a224e607c68fc56c1ff5ee21385e7cf4c4bf44f38c1f1e88a1dde3c3cf5af3375720445c6c0932b63bdbdce7e467082b831fe4b4fc4623358")
  else()
    # just default to x64 otherwise
    set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Windows-x86_64.exe")
    set(sentry_cli_dl_sha512 "7e1ad84df236240694d05dbaed110f9ea85a6088dd4a97888591d35c70ee533ad208896e296c3f93f9f9826c849de9c3e15b23400be6e8ebcaf089a1aec2019b")
  endif()
  set(_exe_suffix ".exe")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(sentry_cli_dl_url "https://github.com/getsentry/sentry-cli/releases/download/${sentry_cli_version}/sentry-cli-Darwin-universal")
  set(sentry_cli_dl_sha512 "847b6a40fb698ae862c9292b77a035bb1a24bc69e0ba3bdcea94d6436038aa1ecf62466a2ba5e81ab2d6fe06fb934e26a1c14a5188549954987d0716bc6f0398")
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
