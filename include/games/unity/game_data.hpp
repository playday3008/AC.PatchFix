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
            std::optional<std::uintptr_t> customize_equip_navcheck;
            std::optional<std::uintptr_t> aim_peak_empty_cb;
            std::optional<std::uintptr_t> aim_peak_working_cb;
            std::optional<std::uintptr_t> aim_peak_sentinel;
            std::optional<std::uintptr_t> freeze_fov_site;
            std::optional<std::uintptr_t> bomb_exit_site;
            std::optional<std::uintptr_t> bomb_onthrow_fn;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="EJECT_WAIT",        .field=&ResolvedAddresses::eject_wait_site,          .offset=0x10, .bytes="C7 81 B8 03 00 00 00 00 80 3E C7 81 E8 03 00 00 ? ? ? ? C3"},
            {.name="CROUCH_TOGGLE",     .field=&ResolvedAddresses::crouch_toggle_site,       .offset=0x13, .bytes="80 7C 24 40 00 75 4C B2 01 48 8B CE E8 ? ? ? ? 84 C0 74 3E"},
            {.name="SWING_TURN_MOVSS",  .field=&ResolvedAddresses::swing_turn_movss,         .offset=0x00, .bytes="F3 0F 10 35 6A 9B D2 02 0F 28 C6"},
            {.name="CUSTOMIZE_NAVCHK",  .field=&ResolvedAddresses::customize_equip_navcheck, .offset=0x00, .bytes="85 D2 75 04 41 C6 00 01 48 8B 49 28 48 8B 49 18"},
            {.name="AIM_PEAK_EMPTY_CB", .field=&ResolvedAddresses::aim_peak_empty_cb,        .offset=0x00, .bytes="41 C6 01 00 C3 CC CC CC"},
            {.name="AIM_PEAK_WORK_CB",  .field=&ResolvedAddresses::aim_peak_working_cb,      .offset=0x00, .bytes="40 53 48 83 EC 20 49 8B D9 E8 A2 4F 03 00 88 03"},
            {.name="AIM_PEAK_SENTINEL", .field=&ResolvedAddresses::aim_peak_sentinel,        .offset=0x00, .bytes="48 89 5C 24 10 57 48 83 EC 70 48 8B 79 28 48 8B D9 48 8B 47 20"},
            {.name="FREEZE_FOV",        .field=&ResolvedAddresses::freeze_fov_site,          .offset=0x00, .bytes="F3 41 0F 11 4E 70 E8 ? ? ? ? 0F 28 00 41 0F 29 86 A0 0A 00 00"},
            {.name="BOMB_EXIT",         .field=&ResolvedAddresses::bomb_exit_site,           .offset=0x00, .bytes="40 53 48 83 EC 40 48 8B 81 88 1C 00 00 48 8B D9 C6 81 79 1B 00 00 00"},
            {.name="BOMB_ONTHROW",      .field=&ResolvedAddresses::bomb_onthrow_fn,          .offset=0x00, .bytes="40 57 48 83 EC 60 48 89 5C 24 78 48 89 6C 24 58 48 8B EA"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Unity>);
} // namespace games
