#pragma once

#include <cstdint>

#include <algorithm>
#include <optional>
#include <vector>

#include <Windows.h>

#include "win32/pe.hpp"

namespace vmp::detail {
    struct VmpSections {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::vector<win32::SectionInfo>   vmp;
        std::optional<win32::SectionInfo> text;
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] auto has_vmp() const -> bool { return !vmp.empty(); }

        [[nodiscard]] auto contains_vmp(std::uintptr_t addr) const -> bool {
            return std::ranges::any_of(vmp,
                                       [addr](const auto &s) -> bool { return s.contains(addr); });
        }
    };

    auto find_vmp_sections(HMODULE hModule) -> VmpSections;
} // namespace vmp::detail
