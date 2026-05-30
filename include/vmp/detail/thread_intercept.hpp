#pragma once

#include <cstdint>

#include <Windows.h>

#include "mem/x64.hpp"
#include "vmp/detail/pe_sections.hpp"

namespace vmp::detail {
    inline auto is_vmp_thread(LPTHREAD_START_ROUTINE start, const VmpSections &sections) -> bool {
        auto addr = reinterpret_cast<uintptr_t>(start);

        if (sections.contains_vmp(addr)) {
            return true;
        }

        if (auto target = mem::x64::branch_target(addr)) {
            if (sections.contains_vmp(*target)) {
                return true;
            }
        }

        return false;
    }
} // namespace vmp::detail
