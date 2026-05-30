#pragma once

#include <cstring>

#include <optional>
#include <string_view>

#include <Windows.h>

namespace vmp::detail {
    struct SectionRange {
        uintptr_t start {};
        uintptr_t end {};

        [[nodiscard]] auto contains(uintptr_t addr) const -> bool {
            return addr >= start && addr < end;
        }
    };

    struct VmpSections {
        std::optional<SectionRange> ubx0;
        std::optional<SectionRange> ubx1;
        std::optional<SectionRange> ubx2;
        SectionRange                text {};
        bool                        has_vmp {};
    };

    inline auto find_section(HMODULE hModule, std::string_view name)
        -> std::optional<SectionRange> {
        auto  base = reinterpret_cast<uintptr_t>(hModule);
        auto *dos  = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        auto *nt   = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(
            base + static_cast<uintptr_t>(dos->e_lfanew));
        auto *sec = IMAGE_FIRST_SECTION(nt);

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            char sec_name[IMAGE_SIZEOF_SHORT_NAME + 1] {};
            std::memcpy(sec_name, sec[i].Name, IMAGE_SIZEOF_SHORT_NAME);

            if (std::string_view(sec_name) == name) {
                auto va = base + sec[i].VirtualAddress;
                return SectionRange {va, va + sec[i].Misc.VirtualSize};
            }
        }
        return std::nullopt;
    }

    inline auto find_vmp_sections(HMODULE hModule) -> VmpSections {
        VmpSections result;

        result.ubx0 = find_section(hModule, ".UBX0");
        result.ubx1 = find_section(hModule, ".UBX1");
        result.ubx2 = find_section(hModule, ".UBX2");

        if (auto text = find_section(hModule, ".text")) {
            result.text = *text;
        }

        result.has_vmp = result.ubx0.has_value();
        return result;
    }
} // namespace vmp::detail
