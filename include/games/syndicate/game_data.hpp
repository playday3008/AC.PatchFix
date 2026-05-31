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
        };

        struct ScanEntry {
            std::string_view              name;
            std::string_view              bytes;
            std::ptrdiff_t                offset;
            std::optional<std::uintptr_t> ResolvedAddresses::*field;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry>({
            {.name="LANG_SETUP",    .bytes="40 55 53 48 8B EC 48 83 EC 48 45 33 C9 48 89 74 24 40 48 8D 15", .offset=0, .field=&ResolvedAddresses::lang_setup},
            {.name="LANG_BF_WRITE", .bytes="89 3D ? ? ? ? 89 1D ? ? ? ? 89 05",                              .offset=0, .field=&ResolvedAddresses::lang_bf_write},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Syndicate>);
} // namespace games
