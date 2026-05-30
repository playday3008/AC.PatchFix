#pragma once

#include <Windows.h>

namespace vmp {
    auto install(HMODULE game_module) -> bool;

    [[nodiscard]] auto active() -> bool;

    void wait_for_unpack();

    void wait_for_integrity_blocked();

    void uninstall();
} // namespace vmp
