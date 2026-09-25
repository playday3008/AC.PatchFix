cmake_minimum_required(VERSION 3.28)

# spdlog's cmake_minimum_required(VERSION 3.10) is the binding constraint
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(PatchFix LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/deps/safetyhook.cmake)
include(cmake/deps/Hooking.Patterns.cmake)
include(cmake/deps/mINI.cmake)
include(cmake/deps/spdlog.cmake)

set(PATCHFIX_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")
set(PATCHFIX_GAMES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games")
include(cmake/PatchFix.cmake)

option(PATCHFIX_WERROR "Treat warnings as errors" OFF)

# clang-cl reads "-Wall" as "-Weverything", and only the frontend variant tells the two clang drivers apart.
set(PATCHFIX_MSVC_FRONTEND $<OR:$<CXX_COMPILER_ID:MSVC>,$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>>)

set(SHARED_COMPILE_OPTIONS
    $<${PATCHFIX_MSVC_FRONTEND}:
        /W4
        /wd4201
        $<$<BOOL:${PATCHFIX_WERROR}>:/WX>
    >
    $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang,GNU>,$<NOT:${PATCHFIX_MSVC_FRONTEND}>>:
        -Wall -Wextra -Wpedantic
        $<$<BOOL:${PATCHFIX_WERROR}>:-Werror>
    >
)

set(SHARED_COMPILE_DEFINITIONS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

add_subdirectory(core)

option(PATCHFIX_BUILD_TESTS "Build unit tests" OFF)
if(PATCHFIX_BUILD_TESTS)
    enable_testing()
    include(cmake/deps/catch2.cmake)
    add_subdirectory(tests)
endif()

# After tests/, so games can add their own test sources to PatchFix.Tests.
add_subdirectory(games)
