cmake_minimum_required(VERSION 3.21)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(AC.PatchFix
    LANGUAGES CXX
    VERSION 3.0.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/deps/injector.cmake)
include(cmake/deps/Hooking.Patterns.cmake)
include(cmake/deps/mINI.cmake)
include(cmake/deps/spdlog.cmake)

set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(SHARED_COMPILE_OPTIONS
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:
        -Wall -Wextra -Wpedantic
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-c++20-compat -Wno-c++20-compat-pedantic
        -Wno-padded
        -Wno-covered-switch-default
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /wd4201
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
)

target_include_directories(patchfix_core PRIVATE "${INC_DIR}")
target_link_libraries(patchfix_core PRIVATE injector HookingPatterns mINI spdlog::spdlog)
target_compile_definitions(patchfix_core PRIVATE ${SHARED_COMPILE_DEFINITIONS})
target_compile_options(patchfix_core PRIVATE ${SHARED_COMPILE_OPTIONS})
set_target_properties(patchfix_core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

function(add_game GAME_TAG)
    string(TOLOWER "${GAME_TAG}" GAME_TAG_LOWER)
    set(GAME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/games/${GAME_TAG_LOWER}")
    file(GLOB_RECURSE GAME_SOURCES "${GAME_SRC_DIR}/*.cpp")

    set(TARGET_NAME "AC.${GAME_TAG}.PatchFix")

    add_library(${TARGET_NAME} SHARED
        ${GAME_SOURCES}
        $<TARGET_OBJECTS:patchfix_core>
    )

    target_include_directories(${TARGET_NAME} PRIVATE "${INC_DIR}")
    target_link_libraries(${TARGET_NAME} PRIVATE injector HookingPatterns mINI spdlog::spdlog)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${SHARED_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${SHARED_COMPILE_OPTIONS})
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "${TARGET_NAME}"
        SUFFIX ".asi"
        PREFIX ""
    )
endfunction()

add_game(Rogue)
