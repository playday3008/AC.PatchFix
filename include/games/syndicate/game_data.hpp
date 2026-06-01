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
            std::optional<std::uintptr_t> mode_insert;
            std::optional<std::uintptr_t> ds4_type_classify;
            std::optional<std::uintptr_t> specs_enumerate_modes;
            std::optional<std::uintptr_t> get_active_device_type;
            std::optional<std::uintptr_t> vehicle_physics_step;
            std::optional<std::uintptr_t> drift_tick_counter;
        };

        struct ScanEntry {
            std::string_view              name;
            std::optional<std::uintptr_t> ResolvedAddresses::*field;
            std::ptrdiff_t                                    offset;
            std::string_view                                  bytes;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry>({
            {.name="LANG_SETUP",             .field=&ResolvedAddresses::lang_setup,             .offset=0, .bytes="40 55 53 48 8B EC 48 83 EC 48 45 33 C9 48 89 74 24 40 48 8D 15"},
            {.name="LANG_BF_WRITE",          .field=&ResolvedAddresses::lang_bf_write,          .offset=0, .bytes="89 3D ? ? ? ? 89 1D ? ? ? ? 89 05"},
            {.name="MODE_INSERT",            .field=&ResolvedAddresses::mode_insert,            .offset=0, .bytes="4C 8B DC 49 89 5B ? 57 48 83 EC ? 4C 8B 41 ? 48 8B 41 ? 48 8B DA 48 8B F9 4C 3B C0 0F 82 ? ? ? ? 4D 85 C0 0F 84 ? ? ? ? 48 8B 49 ? 48 3B D1 0F 82 ? ? ? ? 48 8D 04 40"},
            {.name="DS4_TYPE_CLASSIFY",      .field=&ResolvedAddresses::ds4_type_classify,      .offset=0, .bytes="B9 C4 05 00 00 66 3B C1 75 ? C7 83 90 07 00 00 0B 00 00 00"},
            {.name="SPECS_ENUM_MODES",       .field=&ResolvedAddresses::specs_enumerate_modes,  .offset=0, .bytes="48 8B C4 55 48 8D 68 ? 48 81 EC ? ? ? ? 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 60 ? 45 33 E4"},
            {.name="GET_ACTIVE_DEVICE_TYPE", .field=&ResolvedAddresses::get_active_device_type, .offset=0, .bytes="48 8B 41 ? 48 8B 50 ? 8B 8A ? ? ? ? 48 8B 82"},
            {.name="VEHICLE_PHYSICS_STEP",   .field=&ResolvedAddresses::vehicle_physics_step,   .offset=0, .bytes="40 55 41 55 48 83 EC ? 48 8B 41 ? 48 8B E9"},
            {.name="DRIFT_TICK_COUNTER",     .field=&ResolvedAddresses::drift_tick_counter,     .offset=0, .bytes="40 53 48 83 EC ? 01 91"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Syndicate>);
} // namespace games
