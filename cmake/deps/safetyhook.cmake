include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG        755a4a08aa7d0d7143331dc854235ebc2d1570f9 # main
    GIT_PROGRESS   TRUE
    PATCH_COMMAND   git reset --hard HEAD
                 && git apply "${CMAKE_SOURCE_DIR}/cmake/patches/safetyhook-nt-protect.patch"
)
FetchContent_MakeAvailable(safetyhook)

get_target_property(_sh_inc safetyhook INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(safetyhook PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
target_include_directories(safetyhook SYSTEM INTERFACE ${_sh_inc})
