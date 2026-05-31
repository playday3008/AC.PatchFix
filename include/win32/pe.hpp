#pragma once

#include <cstdint>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Windows.h>

namespace win32 {
    struct SectionInfo {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::string    name;
        std::uintptr_t base {};
        std::uintptr_t size {};
        std::uint32_t  characteristics {};
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] auto end() const -> std::uintptr_t { return base + size; }

        [[nodiscard]] auto is_executable() const -> bool {
            return (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        }

        [[nodiscard]] auto is_writable() const -> bool {
            return (characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        }

        [[nodiscard]] auto contains(std::uintptr_t addr) const -> bool {
            return addr >= base && addr < end();
        }
    };

    auto enumerate_sections(HMODULE hModule = nullptr) -> std::vector<SectionInfo>;
    auto find_section(HMODULE hModule, std::string_view name) -> std::optional<SectionInfo>;
    auto get_module_path(HMODULE hModule) -> std::filesystem::path;
} // namespace win32
