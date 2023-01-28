add_library(crashpad_handler_lib STATIC
    crash_report_upload_thread.cc
    crash_report_upload_thread.h
    handler_main.cc
    handler_main.h
    minidump_to_upload_parameters.cc
    minidump_to_upload_parameters.h
    prune_crash_reports_thread.cc
    prune_crash_reports_thread.h
    user_stream_data_source.cc
    user_stream_data_source.h
)

if(APPLE)
    target_sources(crashpad_handler_lib PRIVATE
        mac/file_limit_annotation.cc
        mac/file_limit_annotation.h
    )
endif()
if(APPLE AND NOT IOS)
    target_sources(crashpad_handler_lib PRIVATE
        mac/crash_report_exception_handler.cc
        mac/crash_report_exception_handler.h
        mac/exception_handler_server.cc
        mac/exception_handler_server.h
    )
endif()

if(LINUX OR ANDROID)
    target_sources(crashpad_handler_lib PRIVATE
        linux/capture_snapshot.cc
        linux/capture_snapshot.h
        linux/crash_report_exception_handler.cc
        linux/crash_report_exception_handler.h
        linux/exception_handler_server.cc
        linux/exception_handler_server.h
    )
endif()

if(LINUX)
    target_sources(crashpad_handler_lib PRIVATE
        linux/cros_crash_report_exception_handler.cc
        linux/cros_crash_report_exception_handler.h
    )
endif()

if(WIN32)
    target_sources(crashpad_handler_lib PRIVATE
        win/crash_report_exception_handler.cc
        win/crash_report_exception_handler.h
    )
endif()

target_link_libraries(crashpad_handler_lib
    PRIVATE
        $<BUILD_INTERFACE:crashpad_interface>
    PUBLIC
        crashpad_compat
        crashpad_minidump
        crashpad_snapshot
        crashpad_util
        mini_chromium
)

if(WIN32)
    if(MSVC)
        target_link_libraries(crashpad_handler_lib PUBLIC crashpad_getopt)
        target_compile_options(crashpad_handler_lib PRIVATE "/wd4201")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(crashpad_handler_lib PRIVATE
        "-Wno-multichar"
    )
endif()

set_property(TARGET crashpad_handler_lib PROPERTY EXPORT_NAME handler)
add_library(crashpad::handler_lib ALIAS crashpad_handler_lib)

crashpad_install_target(crashpad_handler_lib)
crashpad_install_dev(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/crashpad/handler"
    FILES_MATCHING PATTERN "*.h"
)

if(NOT IOS)
    add_executable(crashpad_handler WIN32
        main.cc
    )

    if(LINUX)
        target_sources(crashpad_handler PRIVATE
            ../client/pthread_create_linux.cc
        )
    endif()

    target_link_libraries(crashpad_handler
        PRIVATE
            $<BUILD_INTERFACE:crashpad_interface>
        PUBLIC
            crashpad_client
            crashpad_getopt
            crashpad_handler_lib
            crashpad_minidump
            crashpad_snapshot
            crashpad_tools
            crashpad_util
            mini_chromium
    )

    if(WIN32)
        if(MSVC)
            target_link_options(crashpad_handler PRIVATE "/SUBSYSTEM:WINDOWS")
        endif()
    endif()

    set_property(TARGET crashpad_handler PROPERTY EXPORT_NAME crashpad_handler)
    add_executable(crashpad::handler ALIAS crashpad_handler)

    install(TARGETS crashpad_handler EXPORT crashpad_export
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if(CRASHPAD_WER_ENABLED)
    add_library(crashpad_wer SHARED
        win/wer/crashpad_wer.cc
        win/wer/crashpad_wer.h
        win/wer/crashpad_wer.def
        win/wer/crashpad_wer_main.cc
        ../util/misc/address_types.h
        ../util/win/address_types.h
        ../util/win/registration_protocol_win.h
    )

    target_link_libraries(crashpad_wer
        PRIVATE
            $<BUILD_INTERFACE:crashpad_interface>
        PUBLIC
            crashpad_compat
    )

    set_property(TARGET crashpad_wer PROPERTY EXPORT_NAME crashpad_wer)
    set_property(TARGET crashpad_wer PROPERTY PREFIX "") # ensure MINGW doesn't prefix "lib" to dll name
    add_library(crashpad::wer ALIAS crashpad_wer)

    install(TARGETS crashpad_wer EXPORT crashpad_export
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()
