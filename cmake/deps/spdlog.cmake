include_guard(GLOBAL)
include(FetchContent)

set(SPDLOG_SYSTEM_INCLUDES ON  CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT  ON  CACHE BOOL "" FORCE)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog)
