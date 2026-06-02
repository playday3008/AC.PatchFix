add_executable(AC.PatchFix.Tests
    test_parsers.cpp
    test_ini_field.cpp
    test_topo_sort.cpp
)

target_link_libraries(AC.PatchFix.Tests PRIVATE
    Catch2::Catch2WithMain
    mINI
)

target_include_directories(AC.PatchFix.Tests PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(AC.PatchFix.Tests PRIVATE ${SHARED_COMPILE_DEFINITIONS})
target_compile_options(AC.PatchFix.Tests PRIVATE ${SHARED_COMPILE_OPTIONS})

set_target_properties(AC.PatchFix.Tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
