# patchfix_add_game(ARCH <x64|x86>)
#
# Call from games/<series>/<game>/CMakeLists.txt after that directory's own
# project(<Series>.<Game> VERSION X.Y.Z). The target is named <series>-<game>
# and builds <Series>.<Game>.PatchFix.asi from src/, with include/ private to it.
# tests/*.cpp, if present, are added to PatchFix.Tests.
function(patchfix_add_game)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "ARCH" "")
    if(NOT ARG_ARCH MATCHES "^(x64|x86)$")
        message(FATAL_ERROR "patchfix_add_game: ARCH must be x64 or x86, got '${ARG_ARCH}'")
    endif()
    if(NOT PROJECT_VERSION)
        message(FATAL_ERROR "patchfix_add_game: ${CMAKE_CURRENT_SOURCE_DIR} needs project(... VERSION X.Y.Z)")
    endif()

    file(RELATIVE_PATH game_path "${PATCHFIX_GAMES_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    string(REPLACE "/" "-" target "${game_path}")

    if(ARG_ARCH STREQUAL "x64")
        set(arch_ptr_size 8)
    else()
        set(arch_ptr_size 4)
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL arch_ptr_size)
        message(STATUS "Skipping ${target}: ${ARG_ARCH} plugin, not this build's architecture")
        return()
    endif()

    set(PATCHFIX_OUTPUT_NAME "${PROJECT_NAME}.PatchFix")
    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    configure_file("${PATCHFIX_CORE_DIR}/plugin_info.hpp.in" "${generated_dir}/plugin_info.hpp" @ONLY)

    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    add_library(${target} SHARED ${sources})
    target_link_libraries(${target} PRIVATE PatchFix.Core)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${generated_dir}")
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${PATCHFIX_OUTPUT_NAME}"
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        SUFFIX ".asi"
        PREFIX ""
    )

    file(GLOB test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    if(TARGET PatchFix.Tests AND test_sources)
        target_sources(PatchFix.Tests PRIVATE ${test_sources})
        target_include_directories(PatchFix.Tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()
endfunction()
