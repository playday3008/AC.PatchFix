cmake_minimum_required(VERSION 3.28)

# spdlog's cmake_minimum_required(VERSION 3.10) is the binding constraint
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(AC.PatchFix
    LANGUAGES CXX
    VERSION 3.3.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/deps/safetyhook.cmake)
include(cmake/deps/Hooking.Patterns.cmake)
include(cmake/deps/mINI.cmake)
include(cmake/deps/spdlog.cmake)

set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

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

function(add_game GAME_TAG)
    string(TOLOWER "${GAME_TAG}" GAME_TAG_LOWER)
    set(GAME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/games/${GAME_TAG_LOWER}")
    file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${GAME_SRC_DIR}/*.cpp")

    set(TARGET_NAME "AC.${GAME_TAG}.PatchFix")

    add_library(${TARGET_NAME} SHARED ${GAME_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE PatchFix.Core)
    target_include_directories(${TARGET_NAME} PRIVATE "${INC_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}")

    set(PATCHFIX_OUTPUT_NAME "${TARGET_NAME}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/core/plugin_info.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/plugin_info.hpp"
        @ONLY
    )
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

option(PATCHFIX_BUILD_TESTS "Build unit tests" OFF)
if(PATCHFIX_BUILD_TESTS)
    enable_testing()
    include(cmake/deps/catch2.cmake)
    add_subdirectory(tests)
endif()
