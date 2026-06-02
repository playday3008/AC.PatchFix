include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.15.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Catch2)

# Promote Catch2 includes to SYSTEM to suppress warnings from its headers.
get_target_property(_c2_inc Catch2 INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(Catch2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
target_include_directories(Catch2 SYSTEM INTERFACE ${_c2_inc})
