#include "patterns/signatures.hpp"

#include <cstddef>
#include <cstdint>

#include <expected>
#include <format>
#include <string>
#include <string_view>

#include <Windows.h>

#include <Hooking.Patterns.h>

#include "win32/pe.hpp"

namespace {
    struct ScanRange {
        std::uintptr_t begin = 0;
        std::uintptr_t end   = 0;
    };

    auto find_text_range() -> ScanRange {
        auto text = win32::find_section(nullptr, ".text");
        if (!text) {
            return {};
        }

        std::uintptr_t           scan_begin = 0;
        std::uintptr_t           scan_end   = 0;
        std::uintptr_t           addr       = text->base;
        std::uintptr_t           limit      = text->end();
        MEMORY_BASIC_INFORMATION mbi;

        while (addr < limit) {
            if (VirtualQuery(reinterpret_cast<const void *>(addr), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            constexpr DWORD exec_mask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                        PAGE_EXECUTE_WRITECOPY;

            if (mbi.State == MEM_COMMIT && (mbi.Protect & exec_mask) != 0) {
                auto region_start = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
                auto region_end   = region_start + mbi.RegionSize;
                if (scan_begin == 0) {
                    scan_begin = region_start;
                }
                scan_end = region_end;
            }

            addr = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        }

        return {.begin = scan_begin, .end = scan_end};
    }
} // namespace

auto patterns::find_unique(std::string_view name, std::string_view pat_str, std::ptrdiff_t offset)
    -> std::expected<std::uintptr_t, std::string> {
    static auto range = find_text_range();

    auto pattern = (range.begin != 0) ? hook::make_range_pattern(range.begin, range.end, pat_str)
                                      : hook::pattern(pat_str);

    if (pattern.size() != 1) {
        return std::unexpected(std::format("Pattern {}: {} (found {})",
                                           name,
                                           pattern.size() == 0 ? "NOT FOUND" : "AMBIGUOUS",
                                           pattern.size()));
    }
    return reinterpret_cast<std::uintptr_t>(pattern.get_first(offset));
}
