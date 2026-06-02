cmake_minimum_required(VERSION 3.21)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(AC.PatchFix
    LANGUAGES CXX
    VERSION 3.0.0
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/core/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/core/version.hpp"
    @ONLY
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/deps/safetyhook.cmake)
include(cmake/deps/Hooking.Patterns.cmake)
include(cmake/deps/mINI.cmake)
include(cmake/deps/spdlog.cmake)

set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

option(PATCHFIX_WERROR "Treat warnings as errors" OFF)

set(SHARED_COMPILE_OPTIONS
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -Wall -Wextra -Wpedantic
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-c++20-compat -Wno-c++20-compat-pedantic
        -Wno-padded
        -Wno-covered-switch-default
        $<$<BOOL:${PATCHFIX_WERROR}>:-Werror>
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /wd4201
        $<$<BOOL:${PATCHFIX_WERROR}>:/WX>
    >
)

set(SHARED_COMPILE_DEFINITIONS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

add_library(patchfix_core OBJECT
    src/logger.cpp
    src/patterns/signatures.cpp
    src/config/file_watcher.cpp
    src/dllmain.cpp
    src/mem/protect.cpp
    src/mem/hook.cpp
    src/vmp/integrity_bypass.cpp
    src/vmp/pe_sections.cpp
    src/win32/pe.cpp
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

target_include_directories(patchfix_core PUBLIC "${INC_DIR}")
target_link_libraries(patchfix_core PUBLIC
    safetyhook
    HookingPatterns
    mINI
    spdlog::spdlog_header_only
)
target_compile_definitions(patchfix_core PUBLIC ${SHARED_COMPILE_DEFINITIONS})
target_compile_options(patchfix_core PUBLIC ${SHARED_COMPILE_OPTIONS})
target_compile_options(patchfix_core PUBLIC
    $<$<CXX_COMPILER_ID:Clang>:/GS>
    $<$<CXX_COMPILER_ID:MSVC>:/GS>
)
target_link_options(patchfix_core PUBLIC
    $<$<CXX_COMPILER_ID:Clang>:/DYNAMICBASE /NXCOMPAT>
    $<$<CXX_COMPILER_ID:MSVC>:/DYNAMICBASE /NXCOMPAT>
)
set_target_properties(patchfix_core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

function(add_game GAME_TAG)
    string(TOLOWER "${GAME_TAG}" GAME_TAG_LOWER)
    set(GAME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games/${GAME_TAG_LOWER}")
    file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${GAME_SRC_DIR}/*.cpp")

    set(TARGET_NAME "AC.${GAME_TAG}.PatchFix")

    add_library(${TARGET_NAME} SHARED ${GAME_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE patchfix_core)
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        SUFFIX ".asi"
        PREFIX ""
    )
endfunction()

add_game(Rogue)
add_game(Syndicate)
