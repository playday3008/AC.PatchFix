#pragma once

#include <cstdint>

#include <Windows.h>

#include "mem/x64.hpp"
#include "vmp/detail/pe_sections.hpp"

namespace vmp::detail {
    inline auto is_vmp_thread(LPTHREAD_START_ROUTINE start, const VmpSections &sections) -> bool {
        auto addr = reinterpret_cast<std::uintptr_t>(start);

        if (sections.contains_vmp(addr)) {
            return true;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (VirtualQuery(reinterpret_cast<const void *>(addr), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        constexpr DWORD exec_mask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                    PAGE_EXECUTE_WRITECOPY;
        if ((mbi.Protect & exec_mask) == 0) {
            return false;
        }

        if (auto target = mem::x64::branch_target(addr)) {
            if (sections.contains_vmp(*target)) {
                return true;
            }
        }

        return false;
    }
} // namespace vmp::detail
