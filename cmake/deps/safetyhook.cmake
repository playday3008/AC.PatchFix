include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG        755a4a08aa7d0d7143331dc854235ebc2d1570f9 # main
    GIT_PROGRESS   TRUE
    PATCH_COMMAND  git reset --hard HEAD
        COMMAND    git apply "${CMAKE_SOURCE_DIR}/cmake/patches/safetyhook-nt-protect.patch"
)

# SafetyHook only needs the Zydis decoder; disable everything else.
set(ZYDIS_FEATURE_ENCODER   OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_FORMATTER OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_KNC       OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_AVX512    OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_SEGMENT   OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(safetyhook)

# Promote safetyhook includes to SYSTEM to suppress warnings from its headers.
get_target_property(_sh_inc safetyhook INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(safetyhook PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
target_include_directories(safetyhook SYSTEM INTERFACE ${_sh_inc})
