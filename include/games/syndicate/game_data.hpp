#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"
#include "games/tags.hpp"

namespace games {
    template<>
    struct game_data<Syndicate> {
        static constexpr std::string_view name     = "Syndicate";
        static constexpr std::string_view exe_name = "ACS.exe";

        static constexpr bool             vmprotect         = true;
        static constexpr std::string_view integrity_section = ".UBX0";

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> lang_setup;
            std::optional<std::uintptr_t> lang_bf_write;
            std::optional<std::uintptr_t> swapchain_resize;
            std::optional<std::uintptr_t> mode_insert;
            std::optional<std::uintptr_t> ds4_type_classify;
        };

        struct ScanEntry {
            std::string_view              name;
            std::string_view              bytes;
            std::ptrdiff_t                offset;
            std::optional<std::uintptr_t> ResolvedAddresses::*field;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry>({
            {.name="LANG_SETUP",       .bytes="40 55 53 48 8B EC 48 83 EC 48 45 33 C9 48 89 74 24 40 48 8D 15",                     .offset=0, .field=&ResolvedAddresses::lang_setup},
            {.name="LANG_BF_WRITE",    .bytes="89 3D ? ? ? ? 89 1D ? ? ? ? 89 05",                                                  .offset=0, .field=&ResolvedAddresses::lang_bf_write},
            {.name="SWAPCHAIN_RESIZE", .bytes="48 89 5C 24 ? 56 48 83 EC ? 48 8B D9 48 8B 89 ? ? ? ? 41 8B F1",                     .offset=0, .field=&ResolvedAddresses::swapchain_resize},
            {.name="MODE_INSERT",      .bytes="4C 8B DC 49 89 5B ? 57 48 83 EC ? 4C 8B 41 ? 48 8B 41 ? 48 8B DA 48 8B F9 4C 3B C0 0F 82 ? ? ? ? 4D 85 C0 0F 84 ? ? ? ? 48 8B 49 ? 48 3B D1", .offset=0, .field=&ResolvedAddresses::mode_insert},
            {.name="DS4_TYPE_CLASSIFY",.bytes="B9 C4 05 00 00 66 3B C1 75 ? C7 83 90 07 00 00 0B 00 00 00",                         .offset=0, .field=&ResolvedAddresses::ds4_type_classify},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Syndicate>);
} // namespace games
