#include "patterns/signatures.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <expected>
#include <format>
#include <string>
#include <string_view>

#include <Windows.h>

#include <Hooking.Patterns.h>

namespace {

    struct ScanRange {
        uintptr_t begin = 0;
        uintptr_t end   = 0;
    };

    auto find_text_range() -> ScanRange {
        auto  base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto *dos  = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        auto *nt   = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(
            base + static_cast<uintptr_t>(dos->e_lfanew));
        auto *sec = IMAGE_FIRST_SECTION(nt);

        uintptr_t text_start = 0;
        uintptr_t text_vsize = 0;

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            char sec_name[IMAGE_SIZEOF_SHORT_NAME + 1] {};
            std::memcpy(sec_name, sec[i].Name, IMAGE_SIZEOF_SHORT_NAME);

            if (std::string_view(sec_name) == ".text") {
                text_start = base + sec[i].VirtualAddress;
                text_vsize = sec[i].Misc.VirtualSize;
                break;
            }
        }

        if (text_start == 0) {
            return {};
        }

        uintptr_t                scan_begin = 0;
        uintptr_t                scan_end   = 0;
        uintptr_t                addr       = text_start;
        uintptr_t                limit      = text_start + text_vsize;
        MEMORY_BASIC_INFORMATION mbi;

        while (addr < limit) {
            if (VirtualQuery(reinterpret_cast<const void *>(addr), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            constexpr DWORD exec_mask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                        PAGE_EXECUTE_WRITECOPY;

            if (mbi.State == MEM_COMMIT && (mbi.Protect & exec_mask) != 0) {
                auto region_start = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                auto region_end   = region_start + mbi.RegionSize;
                if (scan_begin == 0) {
                    scan_begin = region_start;
                }
                scan_end = region_end;
            }

            addr = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        }

        return {scan_begin, scan_end};
    }

} // namespace

auto patterns::find_unique(std::string_view name, std::string_view pat_str, ptrdiff_t offset)
    -> std::expected<uintptr_t, std::string> {
    static auto range = find_text_range();

    auto pattern = (range.begin != 0) ? hook::make_range_pattern(range.begin, range.end, pat_str)
                                      : hook::pattern(pat_str);

    if (pattern.size() != 1) {
        return std::unexpected(std::format("Pattern {}: {} (found {})",
                                           name,
                                           pattern.size() == 0 ? "NOT FOUND" : "AMBIGUOUS",
                                           pattern.size()));
    }
    return reinterpret_cast<uintptr_t>(pattern.get_first(offset));
}
