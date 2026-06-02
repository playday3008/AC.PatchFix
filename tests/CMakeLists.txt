add_executable(patchfix_tests
    test_parsers.cpp
    test_ini_field.cpp
    test_topo_sort.cpp
)

target_link_libraries(patchfix_tests PRIVATE
    Catch2::Catch2WithMain
    mINI
)

target_include_directories(patchfix_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(patchfix_tests PRIVATE ${SHARED_COMPILE_DEFINITIONS})
target_compile_options(patchfix_tests PRIVATE ${SHARED_COMPILE_OPTIONS})

set_target_properties(patchfix_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
