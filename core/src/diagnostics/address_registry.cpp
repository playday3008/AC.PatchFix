#include "core/diagnostics/address_registry.hpp"

#include <cstdint>

#include <atomic>

#include <Windows.h>

namespace diagnostics {
    namespace {
        std::atomic<std::uintptr_t> g_module_base {0};
        std::atomic<std::uintptr_t> g_module_end {0};
    } // namespace

    void register_plugin_module(HMODULE hModule) {
        auto        base = reinterpret_cast<std::uintptr_t>(hModule);
        const auto *dos  = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return;
        }
        const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(
            base + static_cast<std::uintptr_t>(dos->e_lfanew));
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            return;
        }
        auto size = static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

        g_module_base.store(base, std::memory_order_release);
        g_module_end.store(base + size, std::memory_order_release);
    }

    auto is_plugin_address(std::uintptr_t addr) -> bool {
        auto base = g_module_base.load(std::memory_order_acquire);
        auto end  = g_module_end.load(std::memory_order_acquire);
        return addr >= base && addr < end;
    }
} // namespace diagnostics
