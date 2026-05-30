#pragma once

#include <cstdint>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Windows.h>

namespace win32 {
    struct SectionInfo {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::string name;
        uintptr_t   base {};
        uintptr_t   size {};
        uint32_t    characteristics {};
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] auto end() const -> uintptr_t { return base + size; }

        [[nodiscard]] auto is_executable() const -> bool {
            return (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        }

        [[nodiscard]] auto is_writable() const -> bool {
            return (characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        }

        [[nodiscard]] auto contains(uintptr_t addr) const -> bool {
            return addr >= base && addr < end();
        }
    };

    inline auto enumerate_sections(HMODULE hModule = nullptr) -> std::vector<SectionInfo> {
        auto base =
            reinterpret_cast<uintptr_t>(hModule != nullptr ? hModule : GetModuleHandleA(nullptr));

        const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        const auto *nt  = reinterpret_cast<const IMAGE_NT_HEADERS *>(
            base + static_cast<uintptr_t>(dos->e_lfanew));

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
        auto sections = std::span(IMAGE_FIRST_SECTION(nt), nt->FileHeader.NumberOfSections);
#pragma clang diagnostic pop

        std::vector<SectionInfo> result;
        result.reserve(sections.size());

        for (const auto &sec : sections) {
            const auto *raw_name = reinterpret_cast<const char *>(sec.Name);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
            auto name = std::string(raw_name, ::strnlen(raw_name, IMAGE_SIZEOF_SHORT_NAME));
#pragma clang diagnostic pop

            result.push_back({
                .name            = std::move(name),
                .base            = base + sec.VirtualAddress,
                .size            = sec.Misc.VirtualSize,
                .characteristics = sec.Characteristics,
            });
        }
        return result;
    }

    inline auto find_section(HMODULE hModule, std::string_view name) -> std::optional<SectionInfo> {
        for (auto &sec : enumerate_sections(hModule)) {
            if (sec.name == name) {
                return sec;
            }
        }
        return std::nullopt;
    }
} // namespace win32
