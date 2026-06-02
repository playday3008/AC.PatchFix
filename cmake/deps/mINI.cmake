include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(mINI
    GIT_REPOSITORY https://github.com/metayeti/mINI.git
    GIT_TAG        0.9.18
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(mINI)

# Promote mINI includes to SYSTEM to suppress warnings from its headers.
get_target_property(_mini_inc mINI INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(mINI PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_mini_inc}")
