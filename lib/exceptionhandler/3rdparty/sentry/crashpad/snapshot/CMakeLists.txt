add_library(crashpad_snapshot STATIC
    annotation_snapshot.cc
    annotation_snapshot.h
    capture_memory.cc
    capture_memory.h
    cpu_architecture.h
    cpu_context.cc
    cpu_context.h
    crashpad_info_client_options.cc
    crashpad_info_client_options.h
    exception_snapshot.h
    handle_snapshot.cc
    handle_snapshot.h
    memory_snapshot.cc
    memory_snapshot.h
    memory_snapshot_generic.h
    minidump/exception_snapshot_minidump.cc
    minidump/exception_snapshot_minidump.h
    minidump/memory_snapshot_minidump.cc
    minidump/memory_snapshot_minidump.h
    minidump/minidump_annotation_reader.cc
    minidump/minidump_annotation_reader.h
    minidump/minidump_context_converter.cc
    minidump/minidump_context_converter.h
    minidump/minidump_simple_string_dictionary_reader.cc
    minidump/minidump_simple_string_dictionary_reader.h
    minidump/minidump_stream.h
    minidump/minidump_string_list_reader.cc
    minidump/minidump_string_list_reader.h
    minidump/minidump_string_reader.cc
    minidump/minidump_string_reader.h
    minidump/module_snapshot_minidump.cc
    minidump/module_snapshot_minidump.h
    minidump/process_snapshot_minidump.cc
    minidump/process_snapshot_minidump.h
    minidump/system_snapshot_minidump.cc
    minidump/system_snapshot_minidump.h
    minidump/thread_snapshot_minidump.cc
    minidump/thread_snapshot_minidump.h
    module_snapshot.h
    process_snapshot.h
    snapshot_constants.h
    system_snapshot.h
    thread_snapshot.h
    unloaded_module_snapshot.cc
    unloaded_module_snapshot.h
)

if(APPLE AND NOT IOS)
    target_sources(crashpad_snapshot PRIVATE
        posix/timezone.cc
        posix/timezone.h
        mac/cpu_context_mac.cc
        mac/cpu_context_mac.h
        mac/exception_snapshot_mac.cc
        mac/exception_snapshot_mac.h
        mac/mach_o_image_annotations_reader.cc
        mac/mach_o_image_annotations_reader.h
        mac/mach_o_image_reader.cc
        mac/mach_o_image_reader.h
        mac/mach_o_image_segment_reader.cc
        mac/mach_o_image_segment_reader.h
        mac/mach_o_image_symbol_table_reader.cc
        mac/mach_o_image_symbol_table_reader.h
        mac/module_snapshot_mac.cc
        mac/module_snapshot_mac.h
        mac/process_reader_mac.cc
        mac/process_reader_mac.h
        mac/process_snapshot_mac.cc
        mac/process_snapshot_mac.h
        mac/process_types.cc
        mac/process_types.h
        mac/process_types/custom.cc
        mac/process_types/flavors.h
        mac/process_types/internal.h
        mac/process_types/traits.h
        mac/system_snapshot_mac.cc
        mac/system_snapshot_mac.h
        mac/thread_snapshot_mac.cc
        mac/thread_snapshot_mac.h
    )
elseif(IOS)
    target_sources(crashpad_snapshot PRIVATE
        posix/timezone.cc
        posix/timezone.h
        ios/exception_snapshot_ios_intermediate_dump.cc
        ios/exception_snapshot_ios_intermediate_dump.h
        ios/intermediate_dump_reader_util.cc
        ios/intermediate_dump_reader_util.h
        ios/memory_snapshot_ios_intermediate_dump.cc
        ios/memory_snapshot_ios_intermediate_dump.h
        ios/module_snapshot_ios_intermediate_dump.cc
        ios/module_snapshot_ios_intermediate_dump.h
        ios/process_snapshot_ios_intermediate_dump.cc
        ios/process_snapshot_ios_intermediate_dump.h
        ios/system_snapshot_ios_intermediate_dump.cc
        ios/system_snapshot_ios_intermediate_dump.h
        ios/thread_snapshot_ios_intermediate_dump.cc
        ios/thread_snapshot_ios_intermediate_dump.h
        mac/cpu_context_mac.cc
        mac/cpu_context_mac.h
    )
else()
    target_sources(crashpad_snapshot PRIVATE
        crashpad_types/crashpad_info_reader.cc
        crashpad_types/crashpad_info_reader.h
    )
endif()

if(LINUX OR ANDROID)
    target_sources(crashpad_snapshot PRIVATE
        posix/timezone.cc
        posix/timezone.h
        linux/capture_memory_delegate_linux.cc
        linux/capture_memory_delegate_linux.h
        linux/cpu_context_linux.cc
        linux/cpu_context_linux.h
        linux/debug_rendezvous.cc
        linux/debug_rendezvous.h
        linux/exception_snapshot_linux.cc
        linux/exception_snapshot_linux.h
        linux/process_reader_linux.cc
        linux/process_reader_linux.h
        linux/process_snapshot_linux.cc
        linux/process_snapshot_linux.h
        linux/signal_context.h
        linux/system_snapshot_linux.cc
        linux/system_snapshot_linux.h
        linux/thread_snapshot_linux.cc
        linux/thread_snapshot_linux.h
        sanitized/memory_snapshot_sanitized.cc
        sanitized/memory_snapshot_sanitized.h
        sanitized/module_snapshot_sanitized.cc
        sanitized/module_snapshot_sanitized.h
        sanitized/process_snapshot_sanitized.cc
        sanitized/process_snapshot_sanitized.h
        sanitized/sanitization_information.cc
        sanitized/sanitization_information.h
        sanitized/thread_snapshot_sanitized.cc
        sanitized/thread_snapshot_sanitized.h
        crashpad_types/image_annotation_reader.cc
        crashpad_types/image_annotation_reader.h
        elf/elf_dynamic_array_reader.cc
        elf/elf_dynamic_array_reader.h
        elf/elf_image_reader.cc
        elf/elf_image_reader.h
        elf/elf_symbol_table_reader.cc
        elf/elf_symbol_table_reader.h
        elf/module_snapshot_elf.cc
        elf/module_snapshot_elf.h
    )
endif()

if(WIN32)
    target_sources(crashpad_snapshot PRIVATE
        win/capture_memory_delegate_win.cc
        win/capture_memory_delegate_win.h
        win/cpu_context_win.cc
        win/cpu_context_win.h
        win/exception_snapshot_win.cc
        win/exception_snapshot_win.h
        win/memory_map_region_snapshot_win.cc
        win/memory_map_region_snapshot_win.h
        win/module_snapshot_win.cc
        win/module_snapshot_win.h
        win/pe_image_annotations_reader.cc
        win/pe_image_annotations_reader.h
        win/pe_image_reader.cc
        win/pe_image_reader.h
        win/pe_image_resource_reader.cc
        win/pe_image_resource_reader.h
        win/process_reader_win.cc
        win/process_reader_win.h
        win/process_snapshot_win.cc
        win/process_snapshot_win.h
        win/process_subrange_reader.cc
        win/process_subrange_reader.h
        win/system_snapshot_win.cc
        win/system_snapshot_win.h
        win/thread_snapshot_win.cc
        win/thread_snapshot_win.h
    )
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(x86)|(i[3-7]86)")
    target_sources(crashpad_snapshot PRIVATE
        x86/cpuid_reader.cc
        x86/cpuid_reader.h
    )
endif()

target_include_directories(crashpad_snapshot INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
target_link_libraries(crashpad_snapshot
    PRIVATE
        $<BUILD_INTERFACE:crashpad_interface>
    PUBLIC
        crashpad_client
        crashpad_compat
        crashpad_util
        mini_chromium
)

if(CRASHPAD_ENABLE_STACKTRACE)
    target_compile_definitions(crashpad_snapshot PRIVATE CLIENT_STACKTRACES_ENABLED)
endif()

if(WIN32)
    target_link_libraries(crashpad_snapshot PRIVATE powrprof)
    if(CRASHPAD_ENABLE_STACKTRACE)
        target_link_libraries(crashpad_snapshot PRIVATE dbghelp)
    endif()
    if(MSVC)
        target_compile_options(crashpad_snapshot PRIVATE "/wd4201")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(crashpad_snapshot PRIVATE
            "-Wno-attributes"
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(crashpad_snapshot PRIVATE
            "-Wno-unknown-attributes"
        )
    endif()

    # Define NTDDI_VERSION >= NTDDI_WIN10_RS5 - so InitializeContext2 definition is always available in process_reader_win.cc
    # NTDDI_WIN10_RS5 = 0x0A000006  /* ABRACADABRA_WIN10_RS5 */
    set_property(
        SOURCE "win/process_reader_win.cc"
        APPEND
        PROPERTY COMPILE_DEFINITIONS
        WINVER=0x0A00 _WIN32_WINNT=0x0A00 NTDDI_VERSION=0x0A000006
    )
endif()

if(APPLE AND NOT IOS AND CRASHPAD_ENABLE_STACKTRACE)
    target_include_directories(crashpad_snapshot PRIVATE ../libunwind/include)
    target_link_libraries(crashpad_snapshot PRIVATE unwind_static)
endif()

if(LINUX AND CRASHPAD_ENABLE_STACKTRACE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UNWIND REQUIRED IMPORTED_TARGET libunwind-ptrace)
    target_link_libraries(crashpad_snapshot PRIVATE PkgConfig::UNWIND)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(crashpad_snapshot PRIVATE
        "-Wno-multichar"
    )
endif()

set_property(TARGET crashpad_snapshot PROPERTY EXPORT_NAME snapshot)
add_library(crashpad::snapshot ALIAS crashpad_snapshot)

crashpad_install_target(crashpad_snapshot)
