#include "core/win32/pe.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <utility>

#include <Windows.h>

#include "core/win32/string.hpp"

namespace win32 {
    auto enumerate_sections(HMODULE hModule) -> std::vector<SectionInfo> {
        auto base = reinterpret_cast<std::uintptr_t>(
            hModule != nullptr ? hModule : GetModuleHandleA(nullptr));

        const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return {};
        }
        const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(
            base + static_cast<std::uintptr_t>(dos->e_lfanew));
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            return {};
        }

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

    auto find_section(HMODULE hModule, std::string_view name) -> std::optional<SectionInfo> {
        for (auto &sec : enumerate_sections(hModule)) {
            if (sec.name == name) {
                return sec;
            }
        }
        return std::nullopt;
    }

    auto get_module_path(HMODULE hModule) -> std::filesystem::path {
        std::array<WCHAR, MAX_PATH> buf {};
        const DWORD                 len = GetModuleFileNameW(hModule, buf.data(), MAX_PATH);
        return {wchar_to_utf8(buf.data(), static_cast<int>(len))};
    }
} // namespace win32
