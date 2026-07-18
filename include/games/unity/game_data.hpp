#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"
#include "games/tags.hpp"

namespace games {
    template<>
    struct game_data<Unity> {
        static constexpr std::string_view name     = "Unity";
        static constexpr std::string_view exe_name = "ACU.exe";

        static constexpr bool             vmprotect         = true;
        static constexpr std::string_view integrity_section = ".UBX0";

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> eject_wait_site;
            std::optional<std::uintptr_t> crouch_toggle_site;
            std::optional<std::uintptr_t> swing_turn_movss;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="EJECT_WAIT",       .field=&ResolvedAddresses::eject_wait_site,    .offset=0x10, .bytes="C7 81 B8 03 00 00 00 00 80 3E C7 81 E8 03 00 00 ? ? ? ? C3"},
            {.name="CROUCH_TOGGLE",    .field=&ResolvedAddresses::crouch_toggle_site, .offset=0x13, .bytes="80 7C 24 40 00 75 4C B2 01 48 8B CE E8 ? ? ? ? 84 C0 74 3E"},
            {.name="SWING_TURN_MOVSS", .field=&ResolvedAddresses::swing_turn_movss,   .offset=0x00, .bytes="F3 0F 10 35 6A 9B D2 02 0F 28 C6"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Unity>);
} // namespace games
