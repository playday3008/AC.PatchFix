#pragma once

#include <stop_token>

#include <Windows.h>

namespace vmp {
    [[nodiscard]] auto install(HMODULE game_module) -> bool;

    [[nodiscard]] auto active() -> bool;

    void wait_for_unpack(std::stop_token stop);

    void wait_for_integrity_blocked(std::stop_token stop);

    void uninstall();
} // namespace vmp
