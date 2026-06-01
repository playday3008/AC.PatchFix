#pragma once

#include <cstddef>
#include <cstdint>

namespace games::syndicate {
    // Display mode entry passed to ModeList_InsertSorted (0x141C72310)
    // 24 bytes: vtable ptr, then 4 u32 fields
    struct ModeEntry {
        std::uintptr_t vtable;       // +0x00
        std::uint32_t  width;        // +0x08
        std::uint32_t  height;       // +0x0C
        std::uint32_t  refresh_rate; // +0x10
        std::uint32_t  format;       // +0x14
    };
    static_assert(0x08 == offsetof(ModeEntry, width));
    static_assert(0x0C == offsetof(ModeEntry, height));
    static_assert(0x10 == offsetof(ModeEntry, refresh_rate));
    static_assert(0x14 == offsetof(ModeEntry, format));
    static_assert(0x18 == sizeof(ModeEntry));
} // namespace games::syndicate
