#pragma once

#include <cstdint>

#include <Windows.h>

#include "vmp/detail/pe_sections.hpp"

namespace vmp::detail {
    inline auto resolve_branch_target(uintptr_t addr) -> uintptr_t {
        auto *bytes = reinterpret_cast<const uint8_t *>(addr);

        if (bytes[0] == 0xE9) {
            auto rel = *reinterpret_cast<const int32_t *>(addr + 1);
            return static_cast<uintptr_t>(static_cast<intptr_t>(addr + 5) + rel);
        }

        if (bytes[0] == 0xE8) {
            auto rel = *reinterpret_cast<const int32_t *>(addr + 1);
            return static_cast<uintptr_t>(static_cast<intptr_t>(addr + 5) + rel);
        }

        if (bytes[0] == 0xFF && bytes[1] == 0x25) {
            auto        disp = *reinterpret_cast<const int32_t *>(addr + 2);
            const auto *ptr  = reinterpret_cast<const uintptr_t *>(
                static_cast<uintptr_t>(static_cast<intptr_t>(addr + 6) + disp));
            return *ptr;
        }

        return 0;
    }

    inline auto is_vmp_thread(LPTHREAD_START_ROUTINE start, const VmpSections &sections) -> bool {
        auto addr = reinterpret_cast<uintptr_t>(start);

        if (sections.ubx0 && sections.ubx0->contains(addr)) {
            return true;
        }

        auto target = resolve_branch_target(addr);
        if (target != 0 && sections.ubx0 && sections.ubx0->contains(target)) {
            return true;
        }

        return false;
    }
} // namespace vmp::detail
