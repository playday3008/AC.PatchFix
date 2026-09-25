add_library(PatchFix.Core OBJECT
    src/logger.cpp
    src/patterns/signatures.cpp
    src/config/file_watcher.cpp
    src/dllmain.cpp
    src/mem/protect.cpp
    src/mem/hook.cpp
    src/vmp/integrity_bypass.cpp
    src/vmp/debug_breakin.cpp
    src/vmp/pe_sections.cpp
    src/win32/pe.cpp
    src/win32/timer_resolution.cpp
    src/diagnostics/stack_walker.cpp
    src/diagnostics/crash_report.cpp
    src/diagnostics/minidump.cpp
    src/diagnostics/address_registry.cpp
    src/diagnostics/crash_handler.cpp
    src/diagnostics/seh_guard.cpp
    src/diagnostics/crash_logger.cpp
    src/diagnostics/patch_registry.cpp
    src/diagnostics/hook_context.cpp
    src/diagnostics/crash_journal.cpp
)

target_include_directories(PatchFix.Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(PatchFix.Core PUBLIC
    safetyhook
    HookingPatterns
    mINI
    spdlog::spdlog_header_only
    winmm
)
target_compile_definitions(PatchFix.Core PUBLIC ${SHARED_COMPILE_DEFINITIONS})
target_compile_options(PatchFix.Core PUBLIC ${SHARED_COMPILE_OPTIONS})
target_compile_options(PatchFix.Core PUBLIC
    $<$<CXX_COMPILER_ID:Clang>:/GS>
    $<$<CXX_COMPILER_ID:MSVC>:/GS>
)
target_link_options(PatchFix.Core PUBLIC
    $<$<CXX_COMPILER_ID:Clang>:/DYNAMICBASE /NXCOMPAT>
    $<$<CXX_COMPILER_ID:MSVC>:/DYNAMICBASE /NXCOMPAT>
)
set_target_properties(PatchFix.Core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_precompile_headers(PatchFix.Core PUBLIC
    <Windows.h>
    <mini/ini.h>
    <spdlog/spdlog.h>
)
