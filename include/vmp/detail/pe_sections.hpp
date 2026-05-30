#pragma once

#include <algorithm>
#include <vector>

#include <optional>

#include <Windows.h>

#include "win32/pe.hpp"

namespace vmp::detail {
    struct VmpSections {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::vector<win32::SectionInfo>   vmp;
        std::optional<win32::SectionInfo> text;
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] auto has_vmp() const -> bool { return !vmp.empty(); }

        [[nodiscard]] auto contains_vmp(uintptr_t addr) const -> bool {
            return std::ranges::any_of(vmp, [addr](const auto &s) { return s.contains(addr); });
        }
    };

    inline auto find_vmp_sections(HMODULE hModule) -> VmpSections {
        VmpSections result;
        for (auto &sec : win32::enumerate_sections(hModule)) {
            if (sec.name.starts_with(".UBX")) {
                result.vmp.push_back(std::move(sec));
            } else if (sec.name == ".text") {
                result.text = std::move(sec);
            }
        }
        return result;
    }
} // namespace vmp::detail
