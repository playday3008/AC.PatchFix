include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(hooking
    GIT_REPOSITORY https://github.com/ThirteenAG/Hooking.Patterns.git
    GIT_TAG        460a47bc813085b66c79768531caab1ed5ea96fc # master
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(hooking)

add_library(HookingPatterns STATIC "${hooking_SOURCE_DIR}/Hooking.Patterns.cpp")
set_property(TARGET HookingPatterns PROPERTY
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(HookingPatterns SYSTEM PUBLIC "${hooking_SOURCE_DIR}")
