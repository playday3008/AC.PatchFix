#pragma once

#include <cstdint>

#include <Windows.h>

namespace diagnostics {
    void register_plugin_module(HMODULE hModule);
    auto is_plugin_address(std::uintptr_t addr) -> bool;
} // namespace diagnostics
