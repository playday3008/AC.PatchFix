#pragma once

#include <cstdint>

enum class FovMode : std::uint8_t {
    Auto     = 0,
    VertPlus = 1,
    HorPlus  = 2,
};

enum class MultiMonitor : std::uint8_t {
    Auto        = 0,
    ForceSingle = 1,
    ForceMulti  = 2,
};
